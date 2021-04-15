/*********************************************************************************************************
**
**                                ���������Ϣ�������޹�˾
**
**                                  ΢�Ͱ�ȫʵʱ����ϵͳ
**
**                                      MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: iot_pi_pro_sx1278.h
**
** ��   ��   ��: Song.xiaolong
**
** �ļ���������: 2021 �� 02 �� 26 ��
**
** ��        ��: IoT Pi Pro LoRa SX1278 ����
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
