/**
  ******************************************************************************
  * @file    iot_pi_pro_spi.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32_drv_spi.c driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef IOT_PI_PRO_SPI_H
#define IOT_PI_PRO_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Definition for SPIX resources */
#define BUS_SPI2                                SPI2
#define BUS_SPI2_CLK_ENABLE()                   __HAL_RCC_SPI2_CLK_ENABLE()
#define BUS_SPI2_CLK_DISABLE()                  __HAL_RCC_SPI2_CLK_DISABLE()

#define BUS_SPI2_MISO_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOC_CLK_ENABLE()
#define BUS_SPI2_MOSI_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOC_CLK_ENABLE()
#define BUS_SPI2_SCK_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOB_CLK_ENABLE()

#define BUS_SPI2_FORCE_RESET()                  __HAL_RCC_SPI2_FORCE_RESET()
#define BUS_SPI2_RELEASE_RESET()                __HAL_RCC_SPI2_RELEASE_RESET()

/* Definition for SPIX Pins */
#define BUS_SPI2_MISO_PIN                       GPIO_PIN_2
#define BUS_SPI2_MISO_GPIO_PORT                 GPIOC
#define BUS_SPI2_MISO_AF                        GPIO_AF5_SPI2

#define BUS_SPI2_MOSI_PIN                       GPIO_PIN_3
#define BUS_SPI2_MOSI_GPIO_PORT                 GPIOC
#define BUS_SPI2_MOSI_AF                        GPIO_AF5_SPI2

#define BUS_SPI2_SCK_PIN                        GPIO_PIN_13
#define BUS_SPI2_SCK_GPIO_PORT                  GPIOB
#define BUS_SPI2_SCK_AF                         GPIO_AF5_SPI2

#ifdef __cplusplus
}
#endif

#endif                                                          /* IOT_PI_PRO_SPI_H                     */
/*********************************************************************************************************
END
*********************************************************************************************************/
