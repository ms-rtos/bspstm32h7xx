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
** 文   件   名: iot_pi_pro_init.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro 初始化
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"
#include "ms_fatfs.h"
#include "ms_net_lwip.h"
#include "ms_net_esp_at.h"
#include "eeprom/ms_drv_xx24xx.h"
#include "sensor/ms_drv_sensor.h"
#include "sensor/ms_drv_ap3216c.h"
#include "sensor/ms_drv_ahtxx.h"
#include "includes.h"
#include "stm32_drv.h"
#include <string.h>

#if BSP_CFG_USE_BOOTLOADER == 0U
/**
 * @brief  System Clock Configuration
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Supply configuration update enable
    */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    /** Configure the main internal regulator output voltage
    */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        while (MS_TRUE);
    }

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                                |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        while (MS_TRUE);
    }
}
#endif

/*
 * @brief delay of nus, SYSTICK is a decrementing counter
 */
void bsp_delay_us(ms_uint32_t nus)
{
    ms_uint32_t ticks;
    ms_uint32_t told, tnow, tcnt = 0;
    ms_uint32_t reload = SysTick->LOAD;
    ms_uint32_t count_per_us;

    /* The count to wait */
    count_per_us = ms_bsp_cpu_freq() / 1000000U;
    ticks = nus * count_per_us;

    /* The counter value when entering delay */
    told = SysTick->VAL;
    while (1) {
        tnow = SysTick->VAL;
        if (tnow != told) {
            if (tnow < told) {
                tcnt += told-tnow;
            } else {
                tcnt += reload-tnow+told;
            }
            told = tnow;

            /* If the time exceeds/equal to the time to be delayed, exit */
            if (tcnt >= ticks) {
                break;
            }
        }
    }
}

/**
 * @brief Initial printk interface.
 *
 * @return N/A
 */
void ms_bsp_printk_init(void)
{
#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    UART_HandleTypeDef UartHandle;

    UartHandle.Instance          = USART1;

    UartHandle.Init.BaudRate     = 115200U;
    UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits     = UART_STOPBITS_1;
    UartHandle.Init.Parity       = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode         = UART_MODE_TX_RX;
    UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&UartHandle);
#endif
}

/**
 * @brief Kernel info print.
 *
 * @param[in] buf Pointer to content which need to be print
 * @param[in] len The length of buffer
 *
 * @return N/A
 */
void ms_bsp_printk(const char *buf, ms_size_t len)
{
#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    ms_size_t i;

    for (i = 0U; i < len; i++) {
        while (!(USART1->ISR & UART_FLAG_TXE)) {
        }
        USART1->TDR = *buf++;
    }
#else
    ms_trace_write(buf, len);
#endif
}

/**
 * @brief Reboot.
 *
 * @return N/A
 */
void ms_bsp_reboot(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();

    while (MS_TRUE);
}

/**
 * @brief Shutdown.
 *
 * @return N/A
 */
void ms_bsp_shutdown(void)
{
    ms_bsp_reboot();
}

#if (MS_CFG_SHELL_MODULE_EN > 0) &&  (BSP_CFG_CONSOLE_DEV != BSP_CONSOLE_NULL)
/**
 * @brief Shell thread.
 *
 * @param[in] arg Shell thread argument
 *
 * @return N/A
 */
static void shell_thread(ms_ptr_t arg)
{
    extern unsigned long __ms_shell_cmd_start__;
    extern unsigned long __ms_shell_cmd_end__;

    ms_shell_io_t bsp_shell_io = {
            (ms_shell_cmd_t *)&__ms_shell_cmd_start__,
            (ms_shell_cmd_t *)&__ms_shell_cmd_end__ - (ms_shell_cmd_t *)&__ms_shell_cmd_start__,
#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_TRACE)
            ms_trace_getc,
            ms_trace_putc,
            ms_trace_write,
            ms_trace_printf,
#elif (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
            ms_getc,
            ms_putc,
            ms_write_stdout,
            ms_printf,
            ms_gets,
#endif
    };

    while (MS_TRUE) {
        ms_shell_enter(&bsp_shell_io);
    }
}
#endif

#if BSP_CFG_ESP8266_EN > 0 && BSP_CFG_ESP8266_UPDATE_FW == 0
#if (BSP_CFG_ESP8266_MODE & BSP_ESP8266_MANUAL_CFG) != 0
/**
 * WiFi AP list.
 */
static const ms_esp_at_net_ap_t ap_list[] = {
    { "Tenda_yu",       "Yubei8686" },
    { "realtek8192",    "123456789" },
    { "ACOINFO",        "yihui87654321" },
};
#endif

/**
 * @brief ESP AT net initial done callback.
 *
 * @param[in] arg Callback function argument
 *
 * @return N/A
 */
static void ms_esp_at_net_init_done(ms_ptr_t arg)
{
#if BSP_CFG_HW_TEST_EN == 0
    ms_err_t ret = MS_ERR;
    ms_esp_at_net_ap_t ap;

#if (BSP_CFG_ESP8266_MODE & BSP_ESP8266_AUTO_JOIN) != 0
    ret = ms_esp_at_auto_join(4U, &ap);
#endif

#if (BSP_CFG_ESP8266_MODE & BSP_ESP8266_MANUAL_CFG) != 0
    if (ret != MS_ERR_NONE) {
        ms_esp_at_connect_to_ap(1U, ap_list, MS_ARRAY_SIZE(ap_list), &ap);
    }
#elif (BSP_CFG_ESP8266_MODE & BSP_ESP8266_SMART_CFG) != 0
    if (ret != MS_ERR_NONE) {
        ms_esp_at_smart_config(40U, &ap);
    }
#else
    (void)ret;
#endif
#endif
}

#if MS_CFG_SHELL_MODULE_EN > 0
/**
 * @brief smartcfg command.
 *
 * @param[in] argc Arguments count
 * @param[in] argv Arguments array
 * @param[in] io Pointer to shell io driver
 *
 * @return N/A
 */
static void __ms_shell_esp8266_smartcfg(int argc, char *argv[], const ms_shell_io_t *io)
{
    ms_esp_at_net_ap_t ap;

    ms_esp_at_smart_config(40U, &ap);
}

MS_SHELL_CMD(smartcfg, __ms_shell_esp8266_smartcfg, "ESP8266 smart configure", __ms_shell_cmd_smartcfg);
#endif
#endif

/**
 * @brief Boot thread.
 *
 * @param[in] arg Boot thread argument
 *
 * @return N/A
 */
static void boot_thread(ms_ptr_t arg)
{
    ms_printk_set_level(MS_PK_INFO);

    ms_pipe_drv_register();
    ms_shm_drv_register();

#if BSP_CFG_GPIO_EN > 0
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    stm32_gpio_drv_register();

    stm32_gpio_dev_create("/dev/lrst",  (ms_addr_t)GPIOA, GPIO_PIN_2);
    stm32_gpio_dev_create("/dev/ldio0", (ms_addr_t)GPIOE, GPIO_PIN_8);
    stm32_gpio_dev_create("/dev/ldio1", (ms_addr_t)GPIOE, GPIO_PIN_4);
    stm32_gpio_dev_create("/dev/ldio2", (ms_addr_t)GPIOB, GPIO_PIN_12);
    stm32_gpio_dev_create("/dev/ldio3", (ms_addr_t)GPIOA, GPIO_PIN_12);
    stm32_gpio_dev_create("/dev/ldio4", (ms_addr_t)GPIOD, GPIO_PIN_4);

    stm32_gpio_dev_create("/dev/led1", (ms_addr_t)GPIOE, GPIO_PIN_0);
    stm32_gpio_dev_create("/dev/led2", (ms_addr_t)GPIOD, GPIO_PIN_2);
    stm32_gpio_dev_create("/dev/led3", (ms_addr_t)GPIOC, GPIO_PIN_12);
    stm32_gpio_dev_create("/dev/key1", (ms_addr_t)GPIOC, GPIO_PIN_13);
    stm32_gpio_dev_create("/dev/key2", (ms_addr_t)GPIOE, GPIO_PIN_3);
    stm32_gpio_dev_create("/dev/key3", (ms_addr_t)GPIOE, GPIO_PIN_1);
#endif

#if BSP_CFG_FB_EN > 0
    stm32_fb_drv_register();
    stm32_fb_dev_register("/dev/fb0");
#endif

#if BSP_CFG_I2C_EN > 0
    stm32_i2c_bus_dev_create("/dev/i2c3", 3U);

    ms_xx24xx_drv_register();
    ms_xx24xx_dev_create("/dev/eeprom", "i2c3", 0x50U, EEPROM_M24C02);
#endif

    gt911_drv_register();
    gt911_dev_create("/dev/touch0", "i2c3");

    ms_ahtxx_drv_register();
    ms_ahtxx_dev_create("/dev/ath20", "i2c3", 0x38U, MS_AHTXX_TYPE_AHT20);

    ms_ap3216c_drv_register();
    ms_ap3216c_dev_create("/dev/ap3216c", "i2c3", 0x1EU);

#if BSP_CFG_TOUCH_EN > 0
    stm32_touch_drv_register();
    stm32_touch_dev_register("/dev/touch0");
#endif

#if BSP_CFG_RTC_EN > 0
    ms_rtc_drv_register();
    stm32_rtc_dev_create();
#endif

#if BSP_CFG_SD_EN > 0
    ms_fatfs_register();
    stm32_sd_drv_register();
#endif

#if BSP_CFG_FLASH_EN > 0
    ms_flashfs_register();
    stm32_flash_mount("/flash");

    ms_apps_update(BSP_CFG_UPDATE_REQUEST_PATH, BSP_CFG_UPDATE_LOG_PATH);
#endif

#if BSP_CFG_ESP8266_EN > 0
#if BSP_CFG_ESP8266_UPDATE_FW == 0
    ms_esp_at_net_init(ms_esp_at_net_init_done, MS_NULL);
#else
    extern void esp8266_update_fw_init(void);
    ms_printk(MS_PK_INFO, "ESP8266 update mode...\n");
    esp8266_update_fw_init();
#endif
#endif

#if BSP_CFG_WDG_EN > 0
    ms_wdg_drv_register();
    ms_wdg_dev_register("/dev/wdg", stm32_wdg_drv());
#endif

#if BSP_CFG_FLASH_EN > 0
    ms_apps_start("ms-boot-param.dtb");
#endif

#if BSP_CFG_SPI_EN > 0
    stm32_spi_bus_dev_create("/dev/spi2", 2U, SPI_POLL_MODE);

    if (sx1278_attach_spi("spi2", "/dev/lora") != MS_ERR_NONE) {
        ms_printk(MS_PK_ERR, "failed to attach spi.\n");
    }
#endif
    ms_process_create("iotpi_sddc", 0x90100000, 131072, 4096, 9, 0 , 0, MS_NULL, MS_NULL, MS_NULL);
}

#if BSP_CFG_HW_TEST_EN > 0

typedef enum {
    TEST_TYPE_EEPROM = 0,
    TEST_TYPE_AHT20,
    TEST_TYPE_AP3216C,
    TEST_TYPE_TOUCHSCREEN,
    TEST_TYPE_SDCARD,
    TEST_TYPE_KEYLED,
    TEST_TYPE_MAX
} test_type_t;

/**
 * @brief Hardware test thread.
 *
 * @param[in] arg Boot thread argument
 *
 * @return N/A
 */
static void test_thread(ms_ptr_t arg)
{
    ms_bool_t is_tests_ok[TEST_TYPE_MAX];

    ms_thread_sleep_s(1);

    memset((ms_ptr_t)is_tests_ok, 0, sizeof(is_tests_ok));

    /*
     * EEPROM test
     */
    ms_printf("**************************************************************************\n");
    ms_printf("                             EEPROM TEST\n");
    ms_printf("**************************************************************************\n");
    {
        int efd = ms_io_open("/dev/eeprom", O_RDWR, 0666);
        ms_eeprom_msg_t emsg;
        ms_uint8_t buf[16];
        ms_uint8_t test_data = 0xa5;
        ms_bool_t is_test_ok = MS_TRUE;

        if (efd > 0) {
            emsg.memaddr = 0;
            emsg.buf = buf;
            emsg.len = sizeof(buf);

            memset(buf, test_data, sizeof(buf));

            if (ms_io_write(efd, &emsg, sizeof(emsg)) == sizeof(emsg)) {
                ms_printf("EEPROM write ok\n");
            } else {
                ms_printf("EEPROM write failed\n");
                is_test_ok = MS_FALSE;
            }

            emsg.memaddr = 0;
            emsg.buf = buf;
            emsg.len = sizeof(buf);

            memset(buf, 0, sizeof(buf));

            if (ms_io_read(efd, &emsg, sizeof(emsg)) == sizeof(emsg)) {
                ms_printf("EEPROM read ok\n");
            } else {
                ms_printf("EEPROM read failed\n");
                is_test_ok = MS_FALSE;
            }

            int i;
            for (i = 0; i < sizeof(buf); i++) {
                if (buf[i] != test_data) {
                    break;
                }
            }

            if (i == sizeof(buf)) {
                ms_printf("EEPROM test ok\n");
            } else {
                ms_printf("EEPROM test failed at %d\n", i);
                is_test_ok = MS_FALSE;
            }

            ms_io_close(efd);

        } else {
            ms_printf("open %s failed\n", "/dev/eeprom");
            is_test_ok = MS_FALSE;
        }

        if (is_test_ok) {
            is_tests_ok[TEST_TYPE_EEPROM] = MS_TRUE;
        } else {
            is_tests_ok[TEST_TYPE_EEPROM] = MS_FALSE;
        }
    }

    /*
     * AHT20 test
     */
    ms_printf("**************************************************************************\n");
    ms_printf("                             AHT20 TEST\n");
    ms_printf("**************************************************************************\n");
    {
        int count = 10;
        double hum_rate, tem_rate;
        ms_sensor_data_t sensor_data[2];
        int fd = ms_io_open("/dev/ath20", O_RDWR, 0666);
        ms_bool_t is_test_ok = MS_TRUE;

        if (fd > 0) {
            memset(sensor_data, 0, sizeof(sensor_data));

            while (--count) {
                sensor_data[0].type = MS_SENSOR_TYPE_TEMP;
                sensor_data[1].type = MS_SENSOR_TYPE_HUMI;

                if (ms_io_read(fd, sensor_data, sizeof(sensor_data)) > 0) {
                    tem_rate = sensor_data[0].u.temp;
                    hum_rate = sensor_data[1].u.humi;
                    printf("hum_rate: %f, tem_rate: %f\n\r", hum_rate, tem_rate);
                } else {
                    ms_printf("read ms_sensor_data_t failed\n");
                    is_test_ok = MS_FALSE;
                }
            }

            ms_io_close(fd);

        } else {
            ms_printf("open %s failed\n", "/dev/ath20");
            is_test_ok = MS_FALSE;
        }

        if (is_test_ok) {
            is_tests_ok[TEST_TYPE_AHT20] = MS_TRUE;
        } else {
            is_tests_ok[TEST_TYPE_AHT20] = MS_FALSE;
        }
    }

    /*
     * AP3216C test
     */
    ms_printf("**************************************************************************\n");
    ms_printf("                             AP3216C TEST\n");
    ms_printf("**************************************************************************\n");
    {
        int count = 10;
        ms_uint32_t ir, als, ps;
        ms_sensor_data_t sensor_data[3];
        int fd = ms_io_open("/dev/ap3216c", O_RDWR, 0666);
        ms_bool_t is_test_ok = MS_TRUE;

        if (fd > 0) {
            memset(sensor_data, 0, sizeof(sensor_data));

            while (--count) {
                sensor_data[0].type = MS_SENSOR_TYPE_IR;
                sensor_data[1].type = MS_SENSOR_TYPE_LIGHT;
                sensor_data[2].type = MS_SENSOR_TYPE_PROXIMITY;

                if (ms_io_read(fd, sensor_data, sizeof(sensor_data)) > 0) {
                    ir  = sensor_data[0].u.ir;
                    ps  = sensor_data[1].u.proximity;
                    als = sensor_data[2].u.light;
                    ms_printf("ir: 0x%08X, ps: 0x%08X, als: 0x%08X\n", ir, ps, als);
                } else {
                    ms_printf("read ms_sensor_data_t failed\n");
                    is_test_ok = MS_FALSE;
                }
            }

            ms_io_close(fd);

        } else {
            ms_printf("open %s failed\n", "/dev/ap3216c");
            is_test_ok = MS_FALSE;
        }

        if (is_test_ok) {
            is_tests_ok[TEST_TYPE_AP3216C] = MS_TRUE;
        } else {
            is_tests_ok[TEST_TYPE_AP3216C] = MS_FALSE;
        }
    }

    /*
     * TS test
     */
    ms_printf("**************************************************************************\n");
    ms_printf("                             TouchScreen TEST\n");
    ms_printf("**************************************************************************\n");
    {
        int i;
        int count = 5;
        ms_uint32_t test_colors[5];
        ms_uint32_t test_x_points[5];
        ms_uint32_t test_y_points[5];
        ms_uint32_t touch_event_cnt;
        ms_touch_event_t touch_event;
        ms_fb_fix_screeninfo_t screeninfo;
        int lcd_fd = ms_io_open("/dev/fb0", O_RDWR, 0666);
        int ts_fd = ms_io_open("/dev/touch0", O_RDWR, 0666);
        ms_bool_t is_test_ok = MS_TRUE;

        if ((lcd_fd > 0) && (ts_fd > 0)) {

            if (ioctl(lcd_fd, MS_FB_CMD_GET_FSCREENINFO, &screeninfo) != MS_ERR_NONE) {
                ms_printf("ioctl: MS_FB_CMD_GET_FSCREENINFO failed!\n");
                is_test_ok = MS_FALSE;
            }
            ms_printf("smem_start: 0x%X, smem_len: %d\n", screeninfo.smem_start, screeninfo.smem_len);

            int index;
            touch_event_cnt  = 0;
            test_colors[0]   = LCD_COLOR_RGB565_RED;
            test_x_points[0] = 0;
            test_y_points[0] = 0;
            test_colors[1]   = LCD_COLOR_RGB565_GREEN;
            test_x_points[1] = 480 - 30;
            test_y_points[1] = 272 - 30;
            test_colors[2]   = LCD_COLOR_RGB565_BLUE;
            test_x_points[2] = 0;
            test_y_points[2] = 272 - 30;
            test_colors[3]   = LCD_COLOR_RGB565_YELLOW;
            test_x_points[3] = 480 - 30;
            test_y_points[3] = 0;
            test_colors[4]   = LCD_COLOR_RGB565_BLACK;
            test_x_points[4] = 240 - 15;
            test_y_points[4] = 136 - 15;

            ms_printf("please press color rectangle.\n");

            index = (touch_event_cnt % (sizeof(test_colors) / sizeof(test_colors[0])));
            BSP_LCD_FillRect(0, 0, 480, 272,  LCD_COLOR_RGB565_WHITE);
            BSP_LCD_FillRect(test_x_points[index], test_y_points[index], 30, 30,  test_colors[index]);

            while (touch_event_cnt < count) {

                if (ms_io_read(ts_fd, &touch_event, sizeof(ms_touch_event_t)) > 0) {
                    for (i = 0; i < touch_event.touch_detected; i++) {
                        if (touch_event.touch_event_id[i] == MS_TOUCH_EVENT_ID_PRESS_DOWN) {
                            touch_event_cnt++;
                            ms_printf("point[%d]: %d, %d\n", i, touch_event.touch_x[i], touch_event.touch_y[i]);

                            if (abs(touch_event.touch_x[i] - test_x_points[index]) > 60 ||
                                abs(touch_event.touch_y[i] - test_y_points[index]) > 60) {
                                is_test_ok = MS_FALSE;
                            }

                            index = (touch_event_cnt % (sizeof(test_colors) / sizeof(test_colors[0])));
                            BSP_LCD_FillRect(0, 0, 480, 272,  LCD_COLOR_RGB565_WHITE);
                            BSP_LCD_FillRect(test_x_points[index], test_y_points[index], 30, 30,  test_colors[index]);
                        }
                    }
                } else {
                    ms_printf("read ms_touch_event_t failed\n");
                    is_test_ok = MS_FALSE;
                }

                ms_thread_sleep_ms(200);
            }

            ms_io_close(lcd_fd);
            ms_io_close(ts_fd);

        } else {
            if (lcd_fd > 0) {
                ms_io_close(lcd_fd);
                ms_printf("open %s failed\n", "/dev/touch0");
                is_test_ok = MS_FALSE;

            } else if (ts_fd > 0) {
                ms_io_close(ts_fd);
                ms_printf("open %s failed\n", "/dev/fb0");
                is_test_ok = MS_FALSE;

            } else {
                ms_printf("open %s and %s failed\n", "/dev/fb0", "/dev/touch0");
                is_test_ok = MS_FALSE;
            }
        }

        if (is_test_ok) {
            is_tests_ok[TEST_TYPE_TOUCHSCREEN] = MS_TRUE;
        } else {
            is_tests_ok[TEST_TYPE_TOUCHSCREEN] = MS_FALSE;
        }
    }

    /*
     * SDCARD test
     */
    ms_printf("**************************************************************************\n");
    ms_printf("                             SDCARD TEST\n");
    ms_printf("**************************************************************************\n");
    {
        MS_DIR *dir;
        ms_dirent_t dirent;
        ms_dirent_t *result;
        int ret;
        ms_bool_t is_test_ok = MS_TRUE;

        dir = ms_io_opendir("/sd0");
        if (dir != MS_NULL) {
            do {
                ret = ms_io_readdir_r(dir, &dirent, &result);
                if ((ret > 0) && (result != MS_NULL)) {
                    if ((strcmp(result->d_name, ".") == 0) || (strcmp(result->d_name, "..") == 0)) {
                        continue;
                    }

                    ms_printf("%s\n", result->d_name);

                } else {
                    break;
                }
            } while (MS_TRUE);

            ms_io_closedir(dir);

        } else {
            ms_printf("SDCARD open failed!\n");
            is_test_ok = MS_FALSE;
        }

        if (is_test_ok) {
            is_tests_ok[TEST_TYPE_SDCARD] = MS_TRUE;
        } else {
            is_tests_ok[TEST_TYPE_SDCARD] = MS_FALSE;
        }
    }

    ms_printf("**************************************************************************\n");
    ms_printf("                             LED KEY TEST\n");
    ms_printf("**************************************************************************\n");
    {
        /* Enable GPIO TX/RX clock */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        typedef struct {
            const char   *name;
            GPIO_TypeDef *gpiox;
            ms_uint16_t   pin;
            ms_bool_t     state;
        } gpio_info_t;

        gpio_info_t gpio_info[] = {
                { "PA2",  GPIOA, GPIO_PIN_2,  MS_TRUE },
                { "PA5",  GPIOA, GPIO_PIN_5,  MS_TRUE },
                { "PA7",  GPIOA, GPIO_PIN_7,  MS_TRUE },
                { "PA12", GPIOA, GPIO_PIN_12, MS_TRUE },
                { "PA15", GPIOA, GPIO_PIN_15, MS_TRUE },

                { "PB5",  GPIOB, GPIO_PIN_5,  MS_TRUE },
                { "PB12", GPIOB, GPIO_PIN_12, MS_TRUE },
                { "PB13", GPIOB, GPIO_PIN_13, MS_TRUE },

                { "PC2",  GPIOC, GPIO_PIN_2,  MS_TRUE },
                { "PC3",  GPIOC, GPIO_PIN_3,  MS_TRUE },
                { "PC4",  GPIOC, GPIO_PIN_4,  MS_TRUE },
                { "PC11", GPIOC, GPIO_PIN_11, MS_TRUE },

                { "PD0",  GPIOD, GPIO_PIN_0,  MS_TRUE },
                { "PD1",  GPIOD, GPIO_PIN_1,  MS_TRUE },
                { "PD4",  GPIOD, GPIO_PIN_4,  MS_TRUE },
                { "PD5",  GPIOD, GPIO_PIN_5,  MS_TRUE },
                { "PD6",  GPIOD, GPIO_PIN_6,  MS_TRUE },
                { "PD12", GPIOD, GPIO_PIN_12, MS_TRUE },
                { "PD13", GPIOD, GPIO_PIN_13, MS_TRUE },

                { "PE4",  GPIOE, GPIO_PIN_4,  MS_TRUE },
                { "PE5",  GPIOE, GPIO_PIN_5,  MS_TRUE },
                { "PE6",  GPIOE, GPIO_PIN_6,  MS_TRUE },
                { "PE7",  GPIOE, GPIO_PIN_7,  MS_TRUE },
                { "PE8",  GPIOE, GPIO_PIN_8,  MS_TRUE },
                { "PE12", GPIOE, GPIO_PIN_12, MS_TRUE },
        };

        int led1 = ms_io_open("/dev/led1", O_WRONLY, 0666);
        int led2 = ms_io_open("/dev/led2", O_WRONLY, 0666);
        int led3 = ms_io_open("/dev/led3", O_WRONLY, 0666);

        int key1 = ms_io_open("/dev/key1", O_WRONLY, 0666);
        int key2 = ms_io_open("/dev/key2", O_WRONLY, 0666);
        int key3 = ms_io_open("/dev/key3", O_WRONLY, 0666);

        ms_gpio_param_t param;

        param.mode  = MS_GPIO_MODE_OUTPUT_PP;
        param.pull  = MS_GPIO_PULL_UP;
        param.speed = MS_GPIO_SPEED_HIGH;
        ms_io_ioctl(led1, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(led2, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(led3, MS_GPIO_CMD_SET_PARAM, &param);

        param.mode  = MS_GPIO_MODE_IRQ_FALLING;
        param.pull  = MS_GPIO_PULL_UP;
        param.speed = MS_GPIO_SPEED_HIGH;
        ms_io_ioctl(key1, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(key2, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(key3, MS_GPIO_CMD_SET_PARAM, &param);

        fd_set rfds;
        struct timeval tv;

        ms_uint8_t led1_val = 0xff;
        ms_uint8_t led2_val = 0xff;
        ms_uint8_t led3_val = 0xff;

        ms_uint8_t test_count = 6;
        ms_bool_t gpio_output_high = MS_FALSE;
        int i = 0;

        for (i = 0; i < MS_ARRAY_SIZE(gpio_info); i++) {
            GPIO_InitTypeDef GPIO_InitStruct;

            bzero(&GPIO_InitStruct, sizeof(GPIO_InitTypeDef));

            GPIO_InitStruct.Pin       = gpio_info[i].pin;
            GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Pull      = GPIO_NOPULL;
            GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
            HAL_GPIO_Init(gpio_info[i].gpiox, &GPIO_InitStruct);
        }

        int next_pressed_key = 1;
        int keys_checked_ok = MS_FALSE;
        int is_test_ok = MS_TRUE;

        ms_printf("please press key-%d\n", next_pressed_key);

        while (test_count > 0) {
            FD_ZERO(&rfds);
            FD_SET(key1, &rfds);
            FD_SET(key2, &rfds);
            FD_SET(key3, &rfds);

            tv.tv_sec = 0;
            tv.tv_usec = 5 * 1000;

            if (select(key3 + 1, &rfds, MS_NULL, MS_NULL, &tv) > 0) {
                if (FD_ISSET(key1, &rfds)) {
                    ms_io_write(led1, &led1_val, sizeof(led1_val));
                    led1_val = ~led1_val;
                    ms_printf("key1 press\n");

                    --test_count;
                    gpio_output_high = (gpio_output_high) ? MS_FALSE: MS_TRUE;

                    if (next_pressed_key == 1) {
                        next_pressed_key++;
                        ms_printf("please press key-%d\n", next_pressed_key);
                    } else {
                        is_test_ok = MS_FALSE;
                        break;
                    }
                }

                if (FD_ISSET(key2, &rfds)) {
                    ms_io_write(led2, &led2_val, sizeof(led2_val));
                    led2_val = ~led2_val;
                    ms_printf("key2 press\n");

                    --test_count;
                    gpio_output_high = (gpio_output_high) ? MS_FALSE: MS_TRUE;

                    if (next_pressed_key == 2) {
                        next_pressed_key++;
                        ms_printf("please press key-%d\n", next_pressed_key);
                    } else {
                        is_test_ok = MS_FALSE;
                        break;
                    }
                }

                if (FD_ISSET(key3, &rfds)) {
                    ms_io_write(led3, &led3_val, sizeof(led3_val));
                    led3_val = ~led3_val;
                    ms_printf("key3 press\n");

                    --test_count;
                    gpio_output_high = (gpio_output_high) ? MS_FALSE: MS_TRUE;

                    if (next_pressed_key == 3) {
                        next_pressed_key = 1;
                        keys_checked_ok = MS_TRUE;
                        ms_printf("please press key-%d\n", next_pressed_key);
                    } else {
                        is_test_ok = MS_FALSE;
                        break;
                    }
                }
            }

            if (gpio_output_high) {
                for (i = 0; i < MS_ARRAY_SIZE(gpio_info); i++) {
                    gpio_info[i].state = MS_TRUE;
                    HAL_GPIO_WritePin(gpio_info[i].gpiox, gpio_info[i].pin, gpio_info[i].state);
                }
            } else {
                for (i = 0; i < MS_ARRAY_SIZE(gpio_info); i++) {
                    gpio_info[i].state = MS_FALSE;
                    HAL_GPIO_WritePin(gpio_info[i].gpiox, gpio_info[i].pin, gpio_info[i].state);
                }
            }
        }

        if (is_test_ok && keys_checked_ok) {
            is_tests_ok[TEST_TYPE_KEYLED] = MS_TRUE;
        } else {
            is_tests_ok[TEST_TYPE_KEYLED] = MS_FALSE;
        }
    }

    ms_printf("**************************************************************************\n");
    ms_printf("                             TEST ITEMS SUMMARY\n");
    ms_printf("**************************************************************************\n");
    {
        int i;

        for (i = 0; i < TEST_TYPE_MAX; i++) {
            switch (i) {
            case TEST_TYPE_EEPROM:
                if (is_tests_ok[i]) {
                    ms_printf("EEPROM \t test OK.\n");
                } else {
                    ms_printf("EEPROM \t test FAIL.\n");
                }
                break;

            case TEST_TYPE_AHT20:
                if (is_tests_ok[i]) {
                    ms_printf("AHT20 \t test OK.\n");
                } else {
                    ms_printf("AHT20 \t test FAIL.\n");
                }
                break;

            case TEST_TYPE_AP3216C:
                if (is_tests_ok[i]) {
                    ms_printf("AP3216C \t test OK.\n");
                } else {
                    ms_printf("AP3216C \t test FAIL.\n");
                }
                break;

            case TEST_TYPE_TOUCHSCREEN:
                if (is_tests_ok[i]) {
                    ms_printf("TOUCHSCREEN \t test OK.\n");
                } else {
                    ms_printf("TOUCHSCREEN \t test FAIL.\n");
                }
                break;

            case TEST_TYPE_SDCARD:
                if (is_tests_ok[i]) {
                    ms_printf("SDCARD \t test OK.\n");
                } else {
                    ms_printf("SDCARD \t test FAIL.\n");
                }
                break;

            case TEST_TYPE_KEYLED:
                if (is_tests_ok[i]) {
                    ms_printf("KEYLED \t test OK.\n");
                } else {
                    ms_printf("KEYLED \t test FAIL.\n");
                }
                break;

            default:
                ms_printf("[WARN]: Some test item lost.\n");
                break;
            }
        }
    }

}
#endif

/**
 * @brief BSP application initial.
 *
 * @return N/A
 */
static void ms_app_init(void)
{
#if (MS_CFG_SHELL_MODULE_EN > 0) &&  (BSP_CFG_CONSOLE_DEV != BSP_CONSOLE_NULL)
    ms_thread_create("t_shell",
                     shell_thread,
                     MS_NULL,
                     2048U,
                     MS_CFG_KERN_LOWEST_PRIO - 1U,
                     0U,
                     MS_THREAD_OPT_SUPER | MS_THREAD_OPT_REENT_EN,
                     MS_NULL);
#endif

    ms_thread_create("t_boot",
                     boot_thread,
                     MS_NULL,
                     4096U,
                     4U,
                     0U,
                     MS_THREAD_OPT_SUPER | MS_THREAD_OPT_REENT_EN,
                     MS_NULL);

#if BSP_CFG_HW_TEST_EN > 0
    ms_thread_create("t_test",
                     test_thread,
                     MS_NULL,
                     2048U,
                     MS_CFG_KERN_LOWEST_PRIO - 2U,
                     0U,
                     MS_THREAD_OPT_SUPER | MS_THREAD_OPT_REENT_EN,
                     MS_NULL);
#endif
}

/**
 * @brief BSP initial.
 *
 * @return N/A
 */
void bsp_init(void)
{
    extern unsigned long __ms_kern_heap_start__;
    extern unsigned long __ms_kern_text_start__;
    extern unsigned long __ms_kern_text_end__;

    ms_mem_layout_t mem_layout[] = {
            [MS_FLASH_REGION] = {
                    BSP_CFG_ROM_BASE,
                    BSP_CFG_ROM_SIZE,
            },
            [MS_KERN_TEXT_REGION] = {
                    (ms_addr_t)&__ms_kern_text_start__,
                    (ms_addr_t)&__ms_kern_text_end__ - (ms_addr_t)&__ms_kern_text_start__,
            },
            [MS_KERN_DATA_REGION] = {
                    BSP_CFG_KERN_RAM_BASE,
                    BSP_CFG_KERN_RAM_SIZE,
            },
            [MS_KERN_HEAP_REGION] = {
                    (ms_addr_t)&__ms_kern_heap_start__,
                    BSP_CFG_KERN_HEAP_SIZE,
            },
            [MS_SHARED_MEM_REGION] = {
                    BSP_CFG_FB_RAM_BASE,
                    BSP_CFG_FB_RAM_SIZE,
            },
            [MS_PROCESS_MEM_REGION] = {
                    BSP_CFG_APP_RAM_BASE,
                    BSP_CFG_APP_RAM_SIZE,
            },
    };

    ms_rtos_init(mem_layout);

#ifdef BSP_CFG_APP2_RAM_BASE
    ms_process_mem_sbrk(BSP_CFG_APP2_RAM_BASE, BSP_CFG_APP2_RAM_SIZE);
#endif

    ms_null_drv_register();

#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    ms_ptr_t pty_dev;
    ms_pty_drv_register();
    ms_pty_dev_create("/dev/console", 128U, &pty_dev);
#elif (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_TRACE)
    ms_trace_drv_register();
    ms_trace_dev_register("/dev/console");
#else
    ms_null_dev_create("/dev/console");
#endif

    ms_stdfile_init();

#if BSP_CFG_UART_EN > 0
    stm32_uart_drv_register();
    stm32_uart_dev_create("/dev/uart0", 1, 256, 256);
#endif

#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    int uart_fd = ms_io_open("/dev/uart0", O_RDWR, 0666);
    ms_uart_param_t param;

    ms_io_ioctl(uart_fd, MS_UART_CMD_GET_PARAM, &param);
    param.baud      = 115200U;
    param.data_bits = MS_UART_DATA_BITS_8B;
    param.stop_bits = MS_UART_STOP_BITS_1B;
    param.parity    = MS_UART_PARITY_NONE;
    ms_io_ioctl(uart_fd, MS_UART_CMD_SET_PARAM, &param);

    ms_pty_dev_start(pty_dev, uart_fd, 1024U, 4U, 0U, 0U);
#endif

    ms_app_init();

    ms_rtos_start();

    while (MS_TRUE) {
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
