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
** 文   件   名: iot_pi_pro_sx1278.h
**
** 创   建   人: Song.xiaolong
**
** 文件创建日期: 2021 年 02 月 26 日
**
** 描        述: IoT Pi Pro LoRa SX1278 驱动
*********************************************************************************************************/

#ifndef IOT_PI_LORA_H
#define IOT_PI_LORA_H

#ifdef __cplusplus
extern "C" {
#endif

#define LORA_NSS_PIN                     GPIO_PIN_5
#define LORA_NSS_GPIO_PORT               GPIOA
#define LOAR_NSS_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOA_CLK_ENABLE();

ms_err_t sx1278_attach_spi(const char *spi_bus_name, const char *dev_name);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  IOT_PI_LORA_H               */
/*********************************************************************************************************
   END
*********************************************************************************************************/
