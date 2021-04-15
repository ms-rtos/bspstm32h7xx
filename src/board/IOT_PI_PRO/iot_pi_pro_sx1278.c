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
** 文   件   名: iot_pi_pro_sx1278.c
**
** 创   建   人: Song.xiaolong
**
** 文件创建日期: 2021 年 02 月 26 日
**
** 描        述: IoT Pi Pro LoRa SX1278 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_kern.h"
#include "ms_io_core.h"
#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"
#include "iot_pi_pro_sx1278.h"

static ms_spi_device_t spi_dev;

/*
 * Nss pin initialize
 */
static void __sx1278_nss_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
	
    LOAR_NSS_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LORA_NSS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LORA_NSS_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LORA_NSS_GPIO_PORT, GPIO_PIN_5, GPIO_PIN_SET);
}

/*
 * CS function
 */
ms_err_t sx1278_cs_func(ms_bool_t select)
{
    if (select == MS_TRUE) {
        HAL_GPIO_WritePin(LORA_NSS_GPIO_PORT, LORA_NSS_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(LORA_NSS_GPIO_PORT, LORA_NSS_PIN, GPIO_PIN_SET);
    }

    return MS_ERR_NONE;
}

/*
 * Attach LoRa to SPI bus device
 */
ms_err_t sx1278_attach_spi(const char *spi_bus_name, const char *dev_name)
{
    ms_err_t err;

    if (spi_bus_name == MS_NULL || dev_name == MS_NULL) {
        return MS_ERR;
    }

    /*
     * Nss pin initialize
     */
    __sx1278_nss_init();

    spi_dev.nnode.name = dev_name;
    spi_dev.cs         = sx1278_cs_func;

    err = ms_spi_device_attach(&spi_dev, spi_bus_name);
    if (err != MS_ERR_NONE) {
        return MS_ERR;
    }

    return MS_ERR_NONE;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
