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
** 文   件   名: iot_pi_pro_i2c.h
**
** 创   建   人: Yu.kangzhi
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro I2C 驱动
*********************************************************************************************************/

#ifndef IOT_PI_PRO_I2C_H
#define IOT_PI_PRO_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  I2C3 for XXX (I2C3: PA8_SCL, PC9_SDA: connect to ext pin)
*********************************************************************************************************/

/* Definition for AUDIO I2Cx resources */
#define BUS_I2C3                                I2C3
#define BUS_I2C3_CLK_ENABLE()                   __HAL_RCC_I2C3_CLK_ENABLE()
#define BUS_I2C3_CLK_DISABLE()                  __HAL_RCC_I2C3_CLK_DISABLE()

#define BUS_I2C3_SCL_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOA_CLK_ENABLE()
#define BUS_I2C3_SDA_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOC_CLK_ENABLE()

#define BUS_I2C3_FORCE_RESET()                  __HAL_RCC_I2C3_FORCE_RESET()
#define BUS_I2C3_RELEASE_RESET()                __HAL_RCC_I2C3_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define BUS_I2C3_SCL_PIN                        GPIO_PIN_8
#define BUS_I2C3_SCL_GPIO_PORT                  GPIOA
#define BUS_I2C3_SCL_AF                         GPIO_AF4_I2C3

#define BUS_I2C3_SDA_PIN                        GPIO_PIN_9
#define BUS_I2C3_SDA_GPIO_PORT                  GPIOC
#define BUS_I2C3_SDA_AF                         GPIO_AF4_I2C3

/* I2C interrupt requests */
#define BUS_I2C3_EV_IRQn                        I2C3_EV_IRQn
#define BUS_I2C3_ER_IRQn                        I2C3_ER_IRQn

/*********************************************************************************************************
  I2C4 for XXX (I2C4: PD12_SCL, PD13_SDA: connect to ext pin)
*********************************************************************************************************/

/* Definition for external, camera and Arduino connector I2Cx resources */
#define BUS_I2C4                                I2C4
#define BUS_I2C4_CLK_ENABLE()                   __HAL_RCC_I2C4_CLK_ENABLE()
#define BUS_I2C4_CLK_DISABLE()                  __HAL_RCC_I2C4_CLK_DISABLE()

#define BUS_I2C4_SCL_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOD_CLK_ENABLE()
#define BUS_I2C4_SDA_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOD_CLK_ENABLE()

#define BUS_I2C4_FORCE_RESET()                  __HAL_RCC_I2C4_FORCE_RESET()
#define BUS_I2C4_RELEASE_RESET()                __HAL_RCC_I2C4_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define BUS_I2C4_SCL_PIN                        GPIO_PIN_12
#define BUS_I2C4_SCL_GPIO_PORT                  GPIOD
#define BUS_I2C4_SCL_AF                         GPIO_AF4_I2C4

#define BUS_I2C4_SDA_PIN                        GPIO_PIN_13
#define BUS_I2C4_SDA_GPIO_PORT                  GPIOD
#define BUS_I2C4_SDA_AF                         GPIO_AF4_I2C4

/* I2C interrupt requests */
#define BUS_I2C4_EV_IRQn                        I2C4_EV_IRQn
#define BUS_I2C4_ER_IRQn                        I2C4_ER_IRQn

/*********************************************************************************************************
  I2C macro defines end
*********************************************************************************************************/

uint32_t BSP_I2C_GetTiming(uint32_t i2c_freq);

#ifdef __cplusplus
}
#endif

#endif                                                          /* IOT_PI_PRO_I2C_H                     */
/*********************************************************************************************************
END
*********************************************************************************************************/
