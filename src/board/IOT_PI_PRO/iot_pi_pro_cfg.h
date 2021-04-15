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
** 文   件   名: iot_pi_pro_cfg.h
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro 配置头文件
*********************************************************************************************************/

#ifndef IOT_PI_PRO_CFG_H
#define IOT_PI_PRO_CFG_H

/*********************************************************************************************************
  运行地址配置
*********************************************************************************************************/

#define BSP_RUN_IN_FLASH            0
#define BSP_RUN_IN_SDRAM            1
#define BSP_RUN_IN_QSPI_FLASH       2

#define BSP_CFG_RUN_IN              BSP_RUN_IN_QSPI_FLASH

/*********************************************************************************************************
  BOOTLOADER 配置
*********************************************************************************************************/

#define BSP_CFG_USE_BOOTLOADER      1

/*********************************************************************************************************
  CPU 主频配置
*********************************************************************************************************/

#define BSP_CFG_CPU_HZ              (400 * 1000 * 1000)
#define BSP_CFG_CPU_HAS_CACHE       1

/*********************************************************************************************************
  功能相关配置
*********************************************************************************************************/

#define BSP_CONSOLE_TRACE           0
#define BSP_CONSOLE_UART            1
#define BSP_CONSOLE_NULL            2

#define BSP_CFG_CONSOLE_DEV         BSP_CONSOLE_TRACE

/*********************************************************************************************************
  地址配置
*********************************************************************************************************/

#define BSP_CFG_FLASH_MEM_BASE      (0x08000000)
#define BSP_CFG_FLASH_MEM_SIZE      (128 * 1024)                /* FLASH BANK1_2: 0x08000000, 128K      */

#define BSP_CFG_QSPI_FLASH_BASE     0x90000000
#define BSP_CFG_QSPI_FLASH_SIZE     (32 * 1024 * 1024)          /* QSPI map area:0x90000000 - 0x9FFFFFFF*/

#define BSP_CFG_DTCM_RAM_BASE       (0x20000000)
#define BSP_CFG_DTCM_RAM_SIZE       (128 * 1024)                /* (128 * 1024):0x20000000 - 0x2001FFFF */

#define BSP_CFG_AXI_SRAM_BASE       (0x24000000)
#define BSP_CFG_AXI_SRAM_SIZE       (512 * 1024)                /* (512 * 1024):0x24000000 - 0x2407FFFF */

#define BSP_CFG_BANK1_3_SRAM_BASE   (0x30000000)
#define BSP_CFG_BANK1_3_SRAM_SIZE   (0x00048000)                /* (256K + 32K):0x30000000 - 0x30047FFF */

#define BSP_CFG_BANK4_SRAM_BASE     (0x38000000)
#define BSP_CFG_BANK4_SRAM_SIZE     (0x00010000)                /* (64 * 1024): 0x38000000 - 0x3800FFFF */

#define BSP_CFG_ROM_BASE            (BSP_CFG_QSPI_FLASH_BASE)
#define BSP_CFG_ROM_SIZE            (BSP_CFG_QSPI_FLASH_SIZE)

#define BSP_CFG_KERN_ROM_BASE       (BSP_CFG_ROM_BASE)
#define BSP_CFG_KERN_ROM_SIZE       (512 * 1024)

#define BSP_CFG_KERN_RAM_BASE       (BSP_CFG_BANK1_3_SRAM_BASE)
#define BSP_CFG_KERN_RAM_SIZE       (256 * 1024)

#define BSP_CFG_FB_RAM_BASE         (BSP_CFG_AXI_SRAM_BASE)
#define BSP_CFG_FB_RAM_SIZE         (256 * 1024)

#define BSP_CFG_APP_RAM_BASE        (BSP_CFG_AXI_SRAM_BASE + 256 * 1024)
#define BSP_CFG_APP_RAM_SIZE        (256 * 1024)

#define BSP_CFG_SHARED_RAM_BASE     (BSP_CFG_BANK1_3_SRAM_BASE + BSP_CFG_KERN_RAM_SIZE)
#define BSP_CFG_SHARED_RAM_SIZE     (32 * 1024)

#define BSP_CFG_KERN_HEAP_SIZE      (64 * 1024)                 /* heap mem locate in kernel RAM area   */
#define BSP_CFG_BOOT_STACK_SIZE     (2048)                      /* stack mem locate in kernel RAM area  */

/*********************************************************************************************************
  升级配置
*********************************************************************************************************/

#define BSP_CFG_UPDATE_REQUEST_PATH "/sd0/update/update_req"
#define BSP_CFG_UPDATE_LOG_PATH     "/sd0/update/update_log"

/*********************************************************************************************************
  驱动配置
*********************************************************************************************************/

#define BSP_CFG_NET_EN              0
#define BSP_CFG_ESP8266_EN          1
#define BSP_CFG_WDG_EN              0
#define BSP_CFG_TOUCH_EN            0
#define BSP_CFG_SD_EN               1
#define BSP_CFG_FB_EN               1
#define BSP_CFG_FLASH_EN            0
#define BSP_CFG_GPIO_EN             1
#define BSP_CFG_I2C_EN              1
#define BSP_CFG_UART_EN             1
#define BSP_CFG_HW_TEST_EN          0
#define BSP_CFG_SPI_EN              1

#if BSP_CFG_HW_TEST_EN > 0
#undef  BSP_CFG_CONSOLE_DEV
#define BSP_CFG_CONSOLE_DEV         BSP_CONSOLE_UART
#endif

/*********************************************************************************************************
  看门狗配置
*********************************************************************************************************/

#define BSP_CFG_WDG_DEADLINE        (5 * MS_CFG_KERN_TICK_HZ)

/*********************************************************************************************************
  ESP8266 配置
*********************************************************************************************************/

#define BSP_ESP8266_AUTO_JOIN       1
#define BSP_ESP8266_SMART_CFG       2
#define BSP_ESP8266_MANUAL_CFG      4

#define BSP_CFG_ESP8266_MODE        (BSP_ESP8266_AUTO_JOIN)

#define BSP_CFG_ESP8266_UPDATE_FW   0

/*********************************************************************************************************
  LCD 配置
*********************************************************************************************************/

#define BSP_CFG_LCD_BPP             16
#define BSP_CFG_LCD_WIDTH           480
#define BSP_CFG_LCD_HEIGHT          272

#define BSP_CFG_FB_LAYER1_ADDR      (BSP_CFG_FB_RAM_BASE)

/*********************************************************************************************************
  RTC 配置
*********************************************************************************************************/

#define BSP_CFG_RTC_ASYNCH_PREDIV  0x7F                     /* LSE as RTC clock                         */
#define BSP_CFG_RTC_SYNCH_PREDIV   0x00FF                   /* LSE as RTC clock                         */

/*********************************************************************************************************
  QSPI FLASH 配置
*********************************************************************************************************/

#define BSP_CFG_QSPI_FLASH_MEM_MAP_EN   1

/*********************************************************************************************************
  FLASH 配置
*********************************************************************************************************/

#define ADDR_FLASH_SECTOR_0                 0x08000000      /* Base address of Sector 0, 128 Kbytes     */

#define BSP_CFG_FLASHFS_MAX_FILE            8
#define BSP_CFG_FLASHFS_UNIT_SIZE           (128 * 1024)

#endif                                                      /* IOT_PI_PRO_CFG_H                         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
