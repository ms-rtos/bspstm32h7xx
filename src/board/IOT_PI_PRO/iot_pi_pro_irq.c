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
** 文   件   名: iot_pi_pro_irq.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro 中断服务程序
*********************************************************************************************************/
#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"

#if BSP_CFG_I2C_EN > 0

extern void stm32_i2c_ev_irqhandler(ms_uint8_t channel);
extern void stm32_i2c_er_irqhandler(ms_uint8_t channel);

void I2C1_EV_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_ev_irqhandler(1U);

    (void)ms_int_exit();
}

void I2C1_ER_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_er_irqhandler(1U);

    (void)ms_int_exit();
}

void I2C2_EV_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_ev_irqhandler(2U);

    (void)ms_int_exit();
}

void I2C2_ER_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_er_irqhandler(2U);

    (void)ms_int_exit();
}

void I2C3_EV_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_ev_irqhandler(3U);

    (void)ms_int_exit();
}

void I2C3_ER_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_er_irqhandler(3U);

    (void)ms_int_exit();
}

void I2C4_EV_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_ev_irqhandler(4U);

    (void)ms_int_exit();
}

void I2C4_ER_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_i2c_er_irqhandler(4U);

    (void)ms_int_exit();
}
#endif

#if BSP_CFG_ESP8266_UPDATE_FW == 0

extern void stm32_uart_irq_handler(ms_uint8_t channel);

/**
 * @brief This function handles USART1 global interrupt.
 */
void USART1_IRQHandler(void)
{
    (void)ms_int_enter();

    stm32_uart_irq_handler(1);

    (void)ms_int_exit();
}
#endif

#if BSP_CFG_SD_EN > 0

extern SD_HandleTypeDef   hsd_sdmmc[SD_INSTANCES_NBR];

/**
 * @brief This function handles SDMMC1 global interrupt.
 */
void SDMMC2_IRQHandler(void)
{
    (void)ms_int_enter();

    BSP_SD_IRQHandler();

    (void)ms_int_exit();
}
#endif

#if BSP_CFG_FB_EN > 0

extern DMA2D_HandleTypeDef  hdma2d;

void DMA2D_IRQHandler(void)
{
    (void)ms_int_enter();

    HAL_DMA2D_IRQHandler(&hdma2d);

    (void)ms_int_exit();
}

extern LTDC_HandleTypeDef   hlcd_ltdc;

void LTDC_IRQHandler(void)
{
    (void)ms_int_enter();

    HAL_LTDC_IRQHandler(&hlcd_ltdc);

    (void)ms_int_exit();
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
