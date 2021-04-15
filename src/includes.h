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
** 文   件   名: includes.h
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: 板级相关头文件
*********************************************************************************************************/

#ifndef __BSP_INCLUDES_H
#define __BSP_INCLUDES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  通用头文件
*********************************************************************************************************/

#include "config.h"

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"
#include "stm32h7xx_hal_usart.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_cortex.h"
#include "stm32h7xx_hal_smartcard.h"
#include "stm32h7xx_hal_dma2d.h"
#include "stm32h7xx_hal_spi.h"
#include "core_cm7.h"

/*********************************************************************************************************
  板级相关头文件
*********************************************************************************************************/

#if defined(STM32H750_DEMO)

#include "stm32h750_demo.h"

#elif defined(IOT_PI_PRO)

#include "iot_pi_pro.h"

#endif

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/

void SystemClock_Config(void);

const ms_flashfs_partition_t *ms_bsp_flash_part_info(void);
ms_uint32_t ms_bsp_flash_data_sector(ms_uint32_t data_sector_id);
ms_uint32_t ms_bsp_flash_addr_to_sector(ms_addr_t addr);

void        ms_bsp_printk_init(void);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __BSP_INCLUDES_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
