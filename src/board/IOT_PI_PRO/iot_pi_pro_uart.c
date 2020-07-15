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
** 文   件   名: iot_pi_pro_uart.c
**
** 创   建   人: Yu.kangzhi
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro USART 驱动
*********************************************************************************************************/

#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"
#include "iot_pi_pro.h"

/*********************************************************************************************************
 *
 * PA9     CH430_RXD       USB串口 RXD       USART1_ TX      AF7
 *
 * PB7     CH430_TXD       USB串口 TXD       USART1_ RX      AF7
 *
 * PC11    EXT-UART4_RX    预留串口4 RX      UART4_RX        AF8
 *
 * PD1     EXT-UART4_TX    预留串口4 TX      UART4_TX        AF8
 *
 * PE7     EXT-UART7_RX    串口7 RX          UART7_RX        AF7
 *
 * PE8     EXT-UART7_TX    串口7 TX          UART7_TX        AF7
 *
 * PC5     WIFI_EN         WIFI使能           ---            --
 *
 * PD8     PD8-USART3_TX   WiFi串口 TX       USART3_ TX      AF7
 *
 * PD9     PD9-USART3_RX   wifi串口 RX       USART3_ RX      AF7
 *
 ********************************************************************************************************/

UART_HandleTypeDef Usart1Handle;
UART_HandleTypeDef Usart3Handle;
UART_HandleTypeDef Uart4Handle;
UART_HandleTypeDef Uart7Handle;

/*********************************************************************************************************
 * @brief UART MSP Initialization
 * @param channel: UART channel
 * @param channel: UART irq pointer
 * @param channel: UART handler pointer pointer
 * @retval error code; 0, success; other, failed
 ********************************************************************************************************/
ms_err_t stm32_uart_get_hw_info(ms_uint8_t channel, IRQn_Type *irq, UART_HandleTypeDef **handler)
{
    if (!irq || !handler) {
        return MS_ERR;
    }

    switch (channel) {
    case 1:
        Usart1Handle.Instance = USART1;
        *irq = USART1_IRQn;
        *handler = &Usart1Handle;
        break;

    case 3:
        Usart3Handle.Instance = USART3;
        *irq = USART1_IRQn;
        *handler = &Usart3Handle;
        break;

    case 4:
        Uart4Handle.Instance = UART4;
        *irq = UART4_IRQn;
        *handler = &Uart4Handle;
        break;

    case 7:
        Uart7Handle.Instance = UART7;
        *irq = UART7_IRQn;
        *handler = &Uart7Handle;
        break;

    default:
        return MS_ERR;
    }

    return MS_ERR_NONE;
}

/*********************************************************************************************************
 * @brief UART MSP Initialization
 *        This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 *           - NVIC configuration for UART interrupt request enable
 * @param huart: UART handle pointer
 * @retval None
 ********************************************************************************************************/
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_Initure;

    if (huart->Instance == USART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();                       /* USART1_TX: PB7, USART1_RX: PA9           */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

        GPIO_Initure.Pin       = GPIO_PIN_9;                /* UART TX RX GPIO pin configuration        */
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_Initure);
        GPIO_Initure.Pin       = GPIO_PIN_7;
        HAL_GPIO_Init(GPIOB, &GPIO_Initure);

    } else if (huart->Instance == USART3) {
        __HAL_RCC_GPIOD_CLK_ENABLE();                       /* USART3_TX: PD8, USART3_RX: PD9           */
        __HAL_RCC_USART3_CLK_ENABLE();

        GPIO_Initure.Pin       = GPIO_PIN_8 | GPIO_PIN_9;   /* UART TX RX GPIO pin configuration        */
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &GPIO_Initure);

    } else if (huart->Instance == UART4) {
        __HAL_RCC_GPIOC_CLK_ENABLE();                       /* UART4_TX: PD1, UART4_RX: PC11            */
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_UART4_CLK_ENABLE();

        GPIO_Initure.Pin       = GPIO_PIN_11;               /* UART TX RX GPIO pin configuration        */
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF8_UART4;
        HAL_GPIO_Init(GPIOC, &GPIO_Initure);
        GPIO_Initure.Pin       = GPIO_PIN_1;
        HAL_GPIO_Init(GPIOD, &GPIO_Initure);

    } else if (huart->Instance == UART7) {
        __HAL_RCC_GPIOE_CLK_ENABLE();                       /* UART7_TX: PE8, UART7_RX: PE7             */
        __HAL_RCC_UART7_CLK_ENABLE();

        GPIO_Initure.Pin       = GPIO_PIN_7 | GPIO_PIN_8;   /* UART TX RX GPIO pin configuration        */
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF7_UART7;
        HAL_GPIO_Init(GPIOE, &GPIO_Initure);

    } else {
        ms_printk(MS_PK_ERR, "This uart not support at the moment!\n\r");
    }
}

/*********************************************************************************************************
 * @brief UART MSP De-Initialization
 *        This function frees the hardware resources used in this example:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO and NVIC configuration to their default state
 * @param huart: UART handle pointer
 * @retval None
 ********************************************************************************************************/
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_FORCE_RESET();                     /* USART1_TX: PB7, USART1_RX: PA9           */
        __HAL_RCC_USART1_RELEASE_RESET();

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_FORCE_RESET();                     /* USART3_TX: PD8, USART3_RX: PD9           */
        __HAL_RCC_USART3_RELEASE_RESET();

        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8 | GPIO_PIN_9);

    } else if (huart->Instance == UART4) {
        __HAL_RCC_UART4_FORCE_RESET();                      /* UART4_TX: PD1, UART4_RX: PC11            */
        __HAL_RCC_UART4_RELEASE_RESET();

        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_11);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_1);

    } else if (huart->Instance == UART7) {
        __HAL_RCC_UART7_FORCE_RESET();                      /* UART7_TX: PE8, UART7_RX: PE7             */
        __HAL_RCC_UART7_RELEASE_RESET();

        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_7 | GPIO_PIN_8);

    } else {
        ms_printk(MS_PK_ERR, "This uart not support at the moment!\n\r");
    }
}

/*********************************************************************************************************
   END
*********************************************************************************************************/
