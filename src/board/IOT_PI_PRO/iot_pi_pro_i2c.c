/*********************************************************************************************************
**
**                                北京翼辉信息技术有限公司
**
**                                  微型安全实时操作系统
**
**                                      MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: iot_pi_pro_i2c.c
**
** 创   建   人: Yu.kangzhi
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro I2C 驱动
*********************************************************************************************************/

#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"
#include "iot_pi_pro.h"

/*********************************************************************************************************
 *
 * PA8      I2C3_SCL    IIC总线 SCL       I2C3_SCL      AF4
 * PC9      I2C3_SDA    IIC总线 SDA       I2C3_SDA      AF4
 *
 * PD12     I2C4_SCL    IIC总线 SCL       I2C4_SCL      AF4
 * PD13     I2C4_SDA    IIC总线 SDA       I2C4_SDA      AF4
 *
 ********************************************************************************************************/

/*********************************************************************************************************
 I2C BUS Private Constants
*********************************************************************************************************/

#ifndef I2C_VALID_TIMING_NBR
  #define I2C_VALID_TIMING_NBR                 128U
#endif
#define I2C_SPEED_FREQ_STANDARD                0U    /* 100 kHz */
#define I2C_SPEED_FREQ_FAST                    1U    /* 400 kHz */
#define I2C_SPEED_FREQ_FAST_PLUS               2U    /* 1 MHz */
#define I2C_ANALOG_FILTER_DELAY_MIN            50U   /* ns */
#define I2C_ANALOG_FILTER_DELAY_MAX            260U  /* ns */
#define I2C_USE_ANALOG_FILTER                  1U
#define I2C_DIGITAL_FILTER_COEF                0U
#define I2C_PRESC_MAX                          16U
#define I2C_SCLDEL_MAX                         16U
#define I2C_SDADEL_MAX                         16U
#define I2C_SCLH_MAX                           256U
#define I2C_SCLL_MAX                           256U
#define SEC2NSEC                               1000000000UL

/*********************************************************************************************************
 I2C BUS Private Types
*********************************************************************************************************/

typedef struct
{
  uint32_t freq;       /* Frequency in Hz */
  uint32_t freq_min;   /* Minimum frequency in Hz */
  uint32_t freq_max;   /* Maximum frequency in Hz */
  uint32_t hddat_min;  /* Minimum data hold time in ns */
  uint32_t vddat_max;  /* Maximum data valid time in ns */
  uint32_t sudat_min;  /* Minimum data setup time in ns */
  uint32_t lscl_min;   /* Minimum low period of the SCL clock in ns */
  uint32_t hscl_min;   /* Minimum high period of SCL clock in ns */
  uint32_t trise;      /* Rise time in ns */
  uint32_t tfall;      /* Fall time in ns */
  uint32_t dnf;        /* Digital noise filter coefficient */
} I2C_Charac_t;

typedef struct
{
  uint32_t presc;      /* Timing prescaler */
  uint32_t tscldel;    /* SCL delay */
  uint32_t tsdadel;    /* SDA delay */
  uint32_t sclh;       /* SCL high period */
  uint32_t scll;       /* SCL low period */
} I2C_Timings_t;

/*********************************************************************************************************
 I2C BUS Private Constants
*********************************************************************************************************/

static uint32_t I2C_GetTiming(uint32_t clock_src_freq, uint32_t i2c_freq);
static uint32_t I2C_Compute_SCLL_SCLH(uint32_t clock_src_freq, uint32_t I2C_speed);
static void     I2C_Compute_PRESC_SCLDEL_SDADEL(uint32_t clock_src_freq, uint32_t I2C_speed);

static I2C_Timings_t I2c_valid_timing[I2C_VALID_TIMING_NBR];
static uint32_t      I2c_valid_timing_nbr = 0;

static const I2C_Charac_t I2C_Charac[] =
{
  [I2C_SPEED_FREQ_STANDARD] =
  {
    .freq = 100000,
    .freq_min = 80000,
    .freq_max = 120000,
    .hddat_min = 0,
    .vddat_max = 3450,
    .sudat_min = 250,
    .lscl_min = 4700,
    .hscl_min = 4000,
    .trise = 640,
    .tfall = 20,
    .dnf = I2C_DIGITAL_FILTER_COEF,
  },
  [I2C_SPEED_FREQ_FAST] =
  {
    .freq = 400000,
    .freq_min = 320000,
    .freq_max = 480000,
    .hddat_min = 0,
    .vddat_max = 900,
    .sudat_min = 100,
    .lscl_min = 1300,
    .hscl_min = 600,
    .trise = 250,
    .tfall = 100,
    .dnf = I2C_DIGITAL_FILTER_COEF,
  },
  [I2C_SPEED_FREQ_FAST_PLUS] =
  {
    .freq = 1000000,
    .freq_min = 800000,
    .freq_max = 1200000,
    .hddat_min = 0,
    .vddat_max = 450,
    .sudat_min = 50,
    .lscl_min = 500,
    .hscl_min = 260,
    .trise = 60,
    .tfall = 100,
    .dnf = I2C_DIGITAL_FILTER_COEF,
  },
};

uint32_t BSP_I2C_GetTiming(uint32_t i2c_freq)
{
    uint32_t timing;

    timing = HAL_RCC_GetPCLK1Freq();

    timing = I2C_GetTiming(HAL_RCC_GetPCLK1Freq(), MS_I2C_CLK_SPEED_STANDARD); //MS_I2C_CLK_SPEED_STANDARD

    return timing;
}

/**
  * @brief  Initializes I2C MSP.
  * @param  hi2c : I2C handler
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef  gpio_init_structure;

    if (hi2c->Instance == I2C3)
    {
        /* Enable SCL GPIO clock */
        BUS_I2C3_SCL_GPIO_CLK_ENABLE();
        /* Enable SDA GPIO clock */
        BUS_I2C3_SDA_GPIO_CLK_ENABLE();

        /* Configure I2C Tx as alternate function */
        gpio_init_structure.Pin       = BUS_I2C3_SCL_PIN;
        gpio_init_structure.Mode      = GPIO_MODE_AF_OD;
        gpio_init_structure.Pull      = GPIO_NOPULL;
        gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = BUS_I2C3_SCL_AF;
        HAL_GPIO_Init(BUS_I2C3_SCL_GPIO_PORT, &gpio_init_structure);

        /* Configure I2C Rx as alternate function */
        gpio_init_structure.Pin       = BUS_I2C3_SDA_PIN;
        gpio_init_structure.Mode      = GPIO_MODE_AF_OD;
        gpio_init_structure.Pull      = GPIO_NOPULL;
        gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = BUS_I2C3_SDA_AF;
        HAL_GPIO_Init(BUS_I2C3_SDA_GPIO_PORT, &gpio_init_structure);

        /*** Configure the I2C peripheral ***/
        /* Enable I2C clock */
        BUS_I2C3_CLK_ENABLE();

        /* Force the I2C peripheral clock reset */
        BUS_I2C3_FORCE_RESET();

        /* Release the I2C peripheral clock reset */
        BUS_I2C3_RELEASE_RESET();
    }
    else if (hi2c->Instance == I2C4)
    {
        /* Enable SCL GPIO clock */
        BUS_I2C4_SCL_GPIO_CLK_ENABLE();
        /* Enable SDA GPIO clock */
        BUS_I2C4_SDA_GPIO_CLK_ENABLE();

        /* Configure I2C Tx as alternate function */
        gpio_init_structure.Pin       = BUS_I2C4_SCL_PIN;
        gpio_init_structure.Mode      = GPIO_MODE_AF_OD;
        gpio_init_structure.Pull      = GPIO_NOPULL;
        gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = BUS_I2C4_SCL_AF;
        HAL_GPIO_Init(BUS_I2C4_SCL_GPIO_PORT, &gpio_init_structure);

        /* Configure I2C Rx as alternate function */
        gpio_init_structure.Pin       = BUS_I2C4_SDA_PIN;
        gpio_init_structure.Mode      = GPIO_MODE_AF_OD;
        gpio_init_structure.Pull      = GPIO_NOPULL;
        gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = BUS_I2C4_SDA_AF;
        HAL_GPIO_Init(BUS_I2C4_SDA_GPIO_PORT, &gpio_init_structure);

        /*** Configure the I2C peripheral ***/
        /* Enable I2C clock */
        BUS_I2C4_CLK_ENABLE();

        /* Force the I2C peripheral clock reset */
        BUS_I2C4_FORCE_RESET();

        /* Release the I2C peripheral clock reset */
        BUS_I2C4_RELEASE_RESET();
    }
    else
    {
        ms_printk(MS_PK_ERR, "This i2c instance not support at the moment!\n\r");
    }
}

/**
  * @brief  DeInitializes I2C MSP.
  * @param  hi2c  I2C handler
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef  gpio_init_structure;

    if (hi2c->Instance == I2C3)
    {
        /* Configure I2C Tx, Rx as alternate function */
        gpio_init_structure.Pin = BUS_I2C3_SCL_PIN;
        HAL_GPIO_DeInit(BUS_I2C3_SCL_GPIO_PORT, gpio_init_structure.Pin );
        gpio_init_structure.Pin = BUS_I2C3_SDA_PIN;
        HAL_GPIO_DeInit(BUS_I2C3_SDA_GPIO_PORT, gpio_init_structure.Pin);

        /* Disable I2C clock */
        BUS_I2C3_CLK_DISABLE();
    }
    else if (hi2c->Instance == I2C4)
    {
        /* Configure I2C Tx, Rx as alternate function */
        gpio_init_structure.Pin = BUS_I2C4_SCL_PIN;
        HAL_GPIO_DeInit(BUS_I2C4_SCL_GPIO_PORT, gpio_init_structure.Pin );
        gpio_init_structure.Pin = BUS_I2C4_SDA_PIN;
        HAL_GPIO_DeInit(BUS_I2C4_SDA_GPIO_PORT, gpio_init_structure.Pin);

        /* Disable I2C clock */
        BUS_I2C4_CLK_DISABLE();
    }
    else
    {
        ms_printk(MS_PK_ERR, "This i2c instance not support at the moment!\n\r");
    }
}

/**
  * @brief  Compute I2C timing according current I2C clock source and required I2C clock.
  * @param  clock_src_freq I2C clock source in Hz.
  * @param  i2c_freq Required I2C clock in Hz.
  * @retval I2C timing or 0 in case of error.
  */
static uint32_t I2C_GetTiming(uint32_t clock_src_freq, uint32_t i2c_freq)
{
  uint32_t ret = 0;
  uint32_t speed;
  uint32_t idx;

  if((clock_src_freq != 0U) && (i2c_freq != 0U))
  {
    for ( speed = 0 ; speed <=  (uint32_t)I2C_SPEED_FREQ_FAST_PLUS ; speed++)
    {
      if ((i2c_freq >= I2C_Charac[speed].freq_min) &&
          (i2c_freq <= I2C_Charac[speed].freq_max))
      {
        I2C_Compute_PRESC_SCLDEL_SDADEL(clock_src_freq, speed);
        idx = I2C_Compute_SCLL_SCLH(clock_src_freq, speed);

        if (idx < I2C_VALID_TIMING_NBR)
        {
          ret = ((I2c_valid_timing[idx].presc  & 0x0FU) << 28) |\
                ((I2c_valid_timing[idx].tscldel & 0x0FU) << 20) |\
                ((I2c_valid_timing[idx].tsdadel & 0x0FU) << 16) |\
                ((I2c_valid_timing[idx].sclh & 0xFFU) << 8) |\
                ((I2c_valid_timing[idx].scll & 0xFFU) << 0);
        }
        break;
      }
    }
  }

  return ret;
}

/**
  * @brief  Compute PRESC, SCLDEL and SDADEL.
  * @param  clock_src_freq I2C source clock in HZ.
  * @param  I2C_speed I2C frequency (index).
  * @retval None.
  */
static void I2C_Compute_PRESC_SCLDEL_SDADEL(uint32_t clock_src_freq, uint32_t I2C_speed)
{
  uint32_t prev_presc = I2C_PRESC_MAX;
  uint32_t ti2cclk;
  int32_t  tsdadel_min, tsdadel_max;
  int32_t  tscldel_min;
  uint32_t presc, scldel, sdadel;
  uint32_t tafdel_min, tafdel_max;

  ti2cclk   = (SEC2NSEC + (clock_src_freq / 2U))/ clock_src_freq;

  tafdel_min = I2C_ANALOG_FILTER_DELAY_MIN;
  tafdel_max = I2C_ANALOG_FILTER_DELAY_MAX;

  /* tDNF = DNF x tI2CCLK
     tPRESC = (PRESC+1) x tI2CCLK
     SDADEL >= {tf +tHD;DAT(min) - tAF(min) - tDNF - [3 x tI2CCLK]} / {tPRESC}
     SDADEL <= {tVD;DAT(max) - tr - tAF(max) - tDNF- [4 x tI2CCLK]} / {tPRESC} */

  tsdadel_min = (int32_t)I2C_Charac[I2C_speed].tfall + (int32_t)I2C_Charac[I2C_speed].hddat_min -
    (int32_t)tafdel_min - (int32_t)(((int32_t)I2C_Charac[I2C_speed].dnf + 3) * (int32_t)ti2cclk);

  tsdadel_max = (int32_t)I2C_Charac[I2C_speed].vddat_max - (int32_t)I2C_Charac[I2C_speed].trise -
    (int32_t)tafdel_max - (int32_t)(((int32_t)I2C_Charac[I2C_speed].dnf + 4) * (int32_t)ti2cclk);


  /* {[tr+ tSU;DAT(min)] / [tPRESC]} - 1 <= SCLDEL */
  tscldel_min = (int32_t)I2C_Charac[I2C_speed].trise + (int32_t)I2C_Charac[I2C_speed].sudat_min;

  if (tsdadel_min <= 0)
  {
    tsdadel_min = 0;
  }

  if (tsdadel_max <= 0)
  {
    tsdadel_max = 0;
  }

  for (presc = 0; presc < I2C_PRESC_MAX; presc++)
  {
    for (scldel = 0; scldel < I2C_SCLDEL_MAX; scldel++)
    {
      /* TSCLDEL = (SCLDEL+1) * (PRESC+1) * TI2CCLK */
      uint32_t tscldel = (scldel + 1U) * (presc + 1U) * ti2cclk;

      if (tscldel >= (uint32_t)tscldel_min)
      {
        for (sdadel = 0; sdadel < I2C_SDADEL_MAX; sdadel++)
        {
          /* TSDADEL = SDADEL * (PRESC+1) * TI2CCLK */
          uint32_t tsdadel = (sdadel * (presc + 1U)) * ti2cclk;

          if ((tsdadel >= (uint32_t)tsdadel_min) && (tsdadel <= (uint32_t)tsdadel_max))
          {
            if(presc != prev_presc)
            {
              I2c_valid_timing[I2c_valid_timing_nbr].presc = presc;
              I2c_valid_timing[I2c_valid_timing_nbr].tscldel = scldel;
              I2c_valid_timing[I2c_valid_timing_nbr].tsdadel = sdadel;
              prev_presc = presc;
              I2c_valid_timing_nbr ++;

              if(I2c_valid_timing_nbr >= I2C_VALID_TIMING_NBR)
              {
                return;
              }
            }
          }
        }
      }
    }
  }
}

/**
  * @brief  Calculate SCLL and SCLH and find best configuration.
  * @param  clock_src_freq I2C source clock in HZ.
  * @param  I2C_speed I2C frequency (index).
  * @retval config index (0 to I2C_VALID_TIMING_NBR], 0xFFFFFFFF for no valid config.
  */
static uint32_t I2C_Compute_SCLL_SCLH (uint32_t clock_src_freq, uint32_t I2C_speed)
{
 uint32_t ret = 0xFFFFFFFFU;
  uint32_t ti2cclk;
  uint32_t ti2cspeed;
  uint32_t prev_error;
  uint32_t dnf_delay;
  uint32_t clk_min, clk_max;
  uint32_t scll, sclh;
  uint32_t tafdel_min;

  ti2cclk   = (SEC2NSEC + (clock_src_freq / 2U))/ clock_src_freq;
  ti2cspeed   = (SEC2NSEC + (I2C_Charac[I2C_speed].freq / 2U))/ I2C_Charac[I2C_speed].freq;

  tafdel_min = I2C_ANALOG_FILTER_DELAY_MIN;

  /* tDNF = DNF x tI2CCLK */
  dnf_delay = I2C_Charac[I2C_speed].dnf * ti2cclk;

  clk_max = SEC2NSEC / I2C_Charac[I2C_speed].freq_min;
  clk_min = SEC2NSEC / I2C_Charac[I2C_speed].freq_max;

  prev_error = ti2cspeed;

  for (uint32_t count = 0; count < I2c_valid_timing_nbr; count++)
  {
    /* tPRESC = (PRESC+1) x tI2CCLK*/
    uint32_t tpresc = (I2c_valid_timing[count].presc + 1U) * ti2cclk;

    for (scll = 0; scll < I2C_SCLL_MAX; scll++)
    {
      /* tLOW(min) <= tAF(min) + tDNF + 2 x tI2CCLK + [(SCLL+1) x tPRESC ] */
      uint32_t tscl_l = tafdel_min + dnf_delay + (2U * ti2cclk) + ((scll + 1U) * tpresc);


      /* The I2CCLK period tI2CCLK must respect the following conditions:
      tI2CCLK < (tLOW - tfilters) / 4 and tI2CCLK < tHIGH */
      if ((tscl_l > I2C_Charac[I2C_speed].lscl_min) && (ti2cclk < ((tscl_l - tafdel_min - dnf_delay) / 4U)))
      {
        for (sclh = 0; sclh < I2C_SCLH_MAX; sclh++)
        {
          /* tHIGH(min) <= tAF(min) + tDNF + 2 x tI2CCLK + [(SCLH+1) x tPRESC] */
          uint32_t tscl_h = tafdel_min + dnf_delay + (2U * ti2cclk) + ((sclh + 1U) * tpresc);

          /* tSCL = tf + tLOW + tr + tHIGH */
          uint32_t tscl = tscl_l + tscl_h + I2C_Charac[I2C_speed].trise + I2C_Charac[I2C_speed].tfall;

          if ((tscl >= clk_min) && (tscl <= clk_max) && (tscl_h >= I2C_Charac[I2C_speed].hscl_min) && (ti2cclk < tscl_h))
          {
            int32_t error = (int32_t)tscl - (int32_t)ti2cspeed;

            if (error < 0)
            {
              error = -error;
            }

            /* look for the timings with the lowest clock error */
            if ((uint32_t)error < prev_error)
            {
              prev_error = (uint32_t)error;
              I2c_valid_timing[count].scll = scll;
              I2c_valid_timing[count].sclh = sclh;
              ret = count;
            }
          }
        }
      }
    }
  }

  return ret;
}

/*********************************************************************************************************
   END
*********************************************************************************************************/
