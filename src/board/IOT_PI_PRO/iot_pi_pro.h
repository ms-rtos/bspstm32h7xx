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
** 文   件   名: iot_pi_pro.h
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro 头文件
*********************************************************************************************************/

#ifndef IOT_PI_PRO_H
#define IOT_PI_PRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iot_pi_pro_sd.h"
#include "iot_pi_pro_lcd.h"
#include "iot_pi_pro_uart.h"
#include "iot_pi_pro_i2c.h"
#include "iot_pi_pro_ts.h"
#include "iot_pi_pro_spi.h"
#include "iot_pi_pro_sx1278.h"

extern void bsp_delay_us(ms_uint32_t nus);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  IOT_PI_PRO_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
