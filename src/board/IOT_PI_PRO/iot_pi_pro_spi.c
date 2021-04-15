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
** ��   ��   ��: iot_pi_pro_spi.c
**
** ��   ��   ��: Song.xiaolong
**
** �ļ���������: 2021 �� 03 �� 20 ��
**
** ��        ��: IoT Pi Pro SPI ����
*********************************************************************************************************/

#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"
#include "iot_pi_pro.h"

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init_structure;

    if (hspi->Instance == BUS_SPI2) {
        BUS_SPI2_SCK_GPIO_CLK_ENABLE();
        BUS_SPI2_MISO_GPIO_CLK_ENABLE();
        BUS_SPI2_MOSI_GPIO_CLK_ENABLE();

        gpio_init_structure.Pin       = BUS_SPI2_SCK_PIN;
        gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
        gpio_init_structure.Pull      = GPIO_NOPULL;
        gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = BUS_SPI2_SCK_AF;
        HAL_GPIO_Init(BUS_SPI2_SCK_GPIO_PORT, &gpio_init_structure);

        gpio_init_structure.Pin       = BUS_SPI2_MISO_PIN;
        gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
        gpio_init_structure.Pull      = GPIO_NOPULL;
        gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = BUS_SPI2_MISO_AF;
        HAL_GPIO_Init(BUS_SPI2_MISO_GPIO_PORT, &gpio_init_structure);

        gpio_init_structure.Pin       = BUS_SPI2_MOSI_PIN;
        gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
        gpio_init_structure.Pull      = GPIO_NOPULL;
        gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = BUS_SPI2_MOSI_AF;
        HAL_GPIO_Init(BUS_SPI2_MOSI_GPIO_PORT, &gpio_init_structure);

        BUS_SPI2_CLK_ENABLE();
        BUS_SPI2_FORCE_RESET();
        BUS_SPI2_RELEASE_RESET();
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == BUS_SPI2) {
        /* Peripheral clock disable */
        BUS_SPI2_CLK_DISABLE();

        HAL_GPIO_DeInit(BUS_SPI2_SCK_GPIO_PORT,  BUS_SPI2_SCK_PIN);
        HAL_GPIO_DeInit(BUS_SPI2_MISO_GPIO_PORT, BUS_SPI2_MISO_PIN);
        HAL_GPIO_DeInit(BUS_SPI2_MOSI_GPIO_PORT, BUS_SPI2_MOSI_PIN);
    }
}
