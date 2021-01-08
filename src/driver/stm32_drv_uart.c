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
** 文   件   名: stm32_drv_uart.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 UART 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"
#include "stm32_drv_uart.h"

#if BSP_CFG_UART_EN > 0

#define __STM32_UART_MAX_CHANNEL                    (6U)

/*
 * Hardware configuration checking
 */
#define __STM32_UART_BAUDRATE_VALID(BAUDRATE)       ((BAUDRATE) < 10500001)

/*
 * Private info
 */
typedef struct {
    ms_pollfd_t          *slots[1U];
    ms_addr_t             base;
    IRQn_Type             irq;
    ms_uart_param_t       param;
    ms_fifo_t             rx_fifo;
    ms_fifo_t             tx_fifo;
    ms_handle_t           rx_semb;
    ms_handle_t           tx_semb;
    ms_tick_t             rx_timeout;
    UART_HandleTypeDef   *uart_handler;
} privinfo_t;

/*
 * stm32 uart device
 */
typedef struct {
    privinfo_t      priv;
    ms_io_device_t  dev;
} stm32_uart_dev_t;

/*
 * @brief Global data area
 */
static stm32_uart_dev_t __stm32_uart_devs[__STM32_UART_MAX_CHANNEL];

/*
 * Enable the peripheral: __HAL_UART_ENABLE(huart);
 */
static inline void __stm32_uart_enable(privinfo_t *priv)
{
    __HAL_UART_ENABLE(priv->uart_handler);
}

/*
 * Disable the peripheral: __HAL_UART_DISABLE(huart);
 */
static inline void __stm32_uart_disable(privinfo_t *priv)
{
    __HAL_UART_DISABLE(priv->uart_handler);
}

/*
 * UART interruption enable/disable: HAL_NVIC_EnableIRQ HAL_NVIC_DisableIRQ
 */
static inline void __stm32_uart_int_set(privinfo_t *priv, ms_uint8_t enable)
{
    if (enable) {
        HAL_NVIC_EnableIRQ(priv->irq);
    } else {
        HAL_NVIC_DisableIRQ(priv->irq);
    }
}

/*
 * UART interruption priority setting: HAL_NVIC_SetPriority
 */
static inline void __stm32_uart_int_prio_set(privinfo_t *priv, ms_uint32_t preempt_prio, ms_uint32_t sub_prio)
{
    HAL_NVIC_SetPriority(priv->irq, preempt_prio, sub_prio);
}

/*
 * UART interruption priority getting: HAL_NVIC_GetPriority
 */
static inline void __stm32_uart_int_prio_get(privinfo_t *priv, ms_uint32_t *preempt_prio, ms_uint32_t *sub_prio)
{
    if (!preempt_prio || !sub_prio) {
        return;
    }
    HAL_NVIC_GetPriority(priv->irq, HAL_NVIC_GetPriorityGrouping(), (uint32_t *)preempt_prio, (uint32_t *)sub_prio);
}

/*
 * UART interruption mode setting: __HAL_UART_ENABLE_IT __HAL_UART_DISABLE_IT
 */
static void __stm32_uart_int_mode_set(privinfo_t *priv, ms_uint32_t interrupt, ms_uint8_t enable)
{
    if (enable) {
        __HAL_UART_ENABLE_IT(priv->uart_handler, interrupt);
    } else {
        __HAL_UART_DISABLE_IT(priv->uart_handler, interrupt);
    }
}

/*
 * UART interruption mode getting: HAL_UART_IRQHandler
 */
static inline ms_uint32_t __stm32_uart_int_mode_get(privinfo_t *priv, ms_uint32_t interrupt)
{
    USART_TypeDef *instance = (USART_TypeDef *)(priv->base);

    return (((((interrupt) >> 28) == 1)? instance->CR1:(((((ms_uint32_t)(interrupt)) >> 28) == 2)? \
             instance->CR2 : instance->CR3)) & (((ms_uint32_t)(interrupt)) & UART_IT_MASK));
}

/*
 * Initialize uart interruption system: HAL_USART_Receive_IT HAL_UART_Receive_IT
 */
static void __stm32_uart_int_config(privinfo_t *priv)
{
    UART_HandleTypeDef *huart = priv->uart_handler;

    /* Process Locked */
    __stm32_uart_int_set(priv, MS_FALSE);

    /* Enable the UART Parity Error Interrupt */
    __HAL_UART_ENABLE_IT(huart, UART_IT_PE);

    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);

    /* Enable the UART Data Register not empty Interrupt */
    __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);

    /* Configure priority */
    __stm32_uart_int_prio_set(priv, 1, 3);

    /* Process Unlocked */
    __stm32_uart_int_set(priv, MS_TRUE);
}

/*********************************************************************************************************
 Above interfaces are hardware relevant
*********************************************************************************************************/

/*
 * Convert ms_uart_param_t to native structure uart_init_t
 */
static int __stm32_uart_param_to_hal_param(ms_uart_param_t *param, UART_InitTypeDef *init)
{
    if (__STM32_UART_BAUDRATE_VALID(param->baud)) {
        init->BaudRate = param->baud;
    } else {
        return MS_ERR;
    }

    if (param->data_bits == MS_UART_DATA_BITS_8B) {
        init->WordLength = UART_WORDLENGTH_8B;
    } else if (param->data_bits == MS_UART_DATA_BITS_9B) {
        init->WordLength = UART_WORDLENGTH_9B;
    } else {
        return MS_ERR;
    }

    if (param->stop_bits == MS_UART_STOP_BITS_1B) {
        init->StopBits = UART_STOPBITS_1;
    } else if (param->stop_bits == MS_UART_STOP_BITS_2B) {
        init->StopBits = UART_STOPBITS_2;
    } else {
        return MS_ERR;
    }

    if (param->parity == MS_UART_PARITY_NONE) {
        init->Parity = UART_PARITY_NONE;
    } else if (param->parity == MS_UART_PARITY_ODD) {
        init->Parity = UART_PARITY_ODD;
    } else if (param->parity == MS_UART_PARITY_EVEN) {
        init->Parity = UART_PARITY_EVEN;
    } else {
        return MS_ERR;
    }

    if (param->flow_ctl == MS_UART_FLOW_CTL_NONE) {
        init->HwFlowCtl = UART_HWCONTROL_NONE;
    } else if (param->flow_ctl == MS_UART_FLOW_CTL_RTS) {
        init->HwFlowCtl = UART_HWCONTROL_RTS;
    } else if (param->flow_ctl == MS_UART_FLOW_CTL_CTS) {
        init->HwFlowCtl = UART_HWCONTROL_CTS;
    } else if (param->flow_ctl == MS_UART_FLOW_CTL_RTS_CTS) {
        init->HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    } else {
        return MS_ERR;
    }

    if (param->mode == MS_UART_MODE_RX) {
        init->Mode = UART_MODE_RX;
    } else if (param->mode == MS_UART_MODE_TX) {
        init->Mode = UART_MODE_TX;
    } else if (param->mode == MS_UART_MODE_TX_RX) {
        init->Mode = UART_MODE_TX_RX;
    } else {
        return MS_ERR;
    }

    /*
     * fix config->word_length
     */
    if (param->data_bits == MS_UART_DATA_BITS_8B &&
        param->parity != MS_UART_PARITY_NONE) {
        init->WordLength = UART_WORDLENGTH_9B;
    }

    return MS_ERR_NONE;
}

/*
 * Convert local param to HAL flags
 */
static ms_err_t __stm32_uart_param_covert(privinfo_t *priv)
{
    UART_HandleTypeDef *uart_handler;

    /* Use no fifo mode by default */
    uart_handler = priv->uart_handler;
    uart_handler->FifoMode = UART_FIFOMODE_DISABLE;

    /* Convert standard parameters to native flags */
    return __stm32_uart_param_to_hal_param(&priv->param, &priv->uart_handler->Init);
}

/*
 * Initialize serial port
 */
static ms_err_t __stm32_uart_hw_init(privinfo_t *priv)
{
    HAL_StatusTypeDef ret;

    ret = HAL_UART_Init(priv->uart_handler);
    if (ret == HAL_OK) {
        return MS_ERR_NONE;
    } else {
        return MS_ERR;
    }
}

/*
 * Open device
 */
static int __stm32_uart_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    privinfo_t *priv = ctx;
    int ret;

    if (ms_atomic_inc(MS_IO_DEV_REF(file)) == 1) {

        priv->param.baud      = 115200;
        priv->param.data_bits = MS_UART_DATA_BITS_8B;
        priv->param.stop_bits = MS_UART_STOP_BITS_1B;
        priv->param.parity    = MS_UART_PARITY_NONE;
        priv->param.flow_ctl  = MS_UART_FLOW_CTL_NONE;
        priv->param.mode      = MS_UART_MODE_TX_RX;

        if (__stm32_uart_param_covert(priv) == MS_ERR_NONE) {
            if (__stm32_uart_hw_init(priv) == MS_ERR_NONE) {
                __stm32_uart_int_config(priv);
                ret = 0;
            } else {
                ms_atomic_dec(MS_IO_DEV_REF(file));
                ms_thread_set_errno(ENODEV);
                ret = -1;
            }
        } else {
            ms_atomic_dec(MS_IO_DEV_REF(file));
            ms_thread_set_errno(EINVAL);
            ret = -1;
        }

    } else {
        ms_atomic_dec(MS_IO_DEV_REF(file));
        ms_thread_set_errno(EBUSY);
        ret = -1;
    }

    return ret;
}

/*
 * Close device
 */
static int __stm32_uart_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    privinfo_t *priv = ctx;

    if (ms_atomic_dec(MS_IO_DEV_REF(file)) == 0) {

        __stm32_uart_int_set(priv, MS_FALSE);
        HAL_UART_DeInit(priv->uart_handler);

        ms_fifo_reset(&priv->rx_fifo);
        ms_fifo_reset(&priv->tx_fifo);

        bzero(&priv->param, sizeof(priv->param));
    }

    return 0;
}

/*
 * Read device
 */
static ms_ssize_t __stm32_uart_read(ms_ptr_t ctx, ms_io_file_t *file, ms_ptr_t buf, ms_size_t len)
{
    privinfo_t *priv = ctx;
    ms_ssize_t ret = 0;
    ms_uint8_t *cbuf = buf;
    ms_uint32_t read_len;

    while (len > 0) {

        if (ms_fifo_is_empty(&priv->rx_fifo)) {
            if (file->flags & FNONBLOCK) {
                break;
            } else {
                if (ms_semb_wait(priv->rx_semb, priv->rx_timeout) != MS_ERR_NONE) {
                    break;
                }
            }
        }

        read_len = ms_fifo_get(&priv->rx_fifo, cbuf, len);
        ret  += read_len;
        len  -= read_len;
        cbuf += read_len;

        break;
    }

    return ret;
}

/*
 * Write device
 */
static ms_ssize_t __stm32_uart_write(ms_ptr_t ctx, ms_io_file_t *file, ms_const_ptr_t buf, ms_size_t len)
{
    ms_arch_sr_t sr;
    privinfo_t *priv = ctx;
    ms_ssize_t ret = 0;
    const ms_uint8_t *cbuf = buf;
    ms_uint32_t write_len;

    while (len > 0) {

        if (ms_fifo_is_full(&priv->tx_fifo)) {
            if (file->flags & FNONBLOCK) {
                break;
            } else {
                (void)ms_semb_wait(priv->tx_semb, MS_TIMEOUT_FOREVER);
            }
        }

        write_len = ms_fifo_put(&priv->tx_fifo, cbuf, len);

        sr = ms_arch_int_disable();
        __stm32_uart_int_mode_set(priv, UART_IT_TXE, MS_TRUE);
        ms_arch_int_resume(sr);

        ret  += write_len;
        len  -= write_len;
        cbuf += write_len;
    }

    return ret;
}

/*
 * Control device
 */
static int __stm32_uart_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    ms_arch_sr_t sr;
    privinfo_t *priv = ctx;
    ms_err_t err;
    int ret = 0;

    switch (cmd) {
    case MS_UART_CMD_GET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_uart_param_t), MS_ACCESS_W)) {
            if (arg != NULL) {
                *(ms_uart_param_t *)arg = priv->param;
                ret = 0;
            } else {
                ms_thread_set_errno(EINVAL);
                ret = -1;
            }
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_UART_CMD_SET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_uart_param_t), MS_ACCESS_R)) {
            if (arg != NULL) {
                priv->param = *(ms_uart_param_t *)arg;

                err = __stm32_uart_param_covert(priv);
                if (err == MS_ERR_NONE) {
                    __stm32_uart_hw_init(priv);
                    ret = 0;
                } else {
                    ms_thread_set_errno(EINVAL);
                    ret = -1;
                }
            } else {
                ms_thread_set_errno(EINVAL);
                ret = -1;
            }
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_UART_CMD_GET_R_TIMEOUT:
        if (ms_access_ok(arg, sizeof(ms_uint32_t), MS_ACCESS_W)) {
            *(ms_uint32_t *)arg = ms_time_tick_to_ms(priv->rx_timeout);
            ret = 0;
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_UART_CMD_SET_R_TIMEOUT:
        if (ms_access_ok(arg, sizeof(ms_uint32_t), MS_ACCESS_R)) {
            priv->rx_timeout = ms_time_ms_to_tick(*(ms_uint32_t *)arg);
            ret = 0;
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_UART_CMD_FLUSH_RX:
        sr = ms_arch_int_disable();
        ms_fifo_reset(&priv->rx_fifo);
        ms_arch_int_resume(sr);
        ret = 0;
        break;

    case MS_UART_CMD_DRAIN_TX:
        while (ms_fifo_is_empty(&priv->tx_fifo) != MS_TRUE) {
            __stm32_uart_int_mode_set(priv, UART_IT_TXE, MS_TRUE);
            (void)ms_semb_wait(priv->tx_semb, MS_TIMEOUT_FOREVER);
        }
        ret = 0;
        break;

    default:
        ms_thread_set_errno(EINVAL);
        ret = -1;
        break;
    }

    return ret;
}

/*
 * Get device status
 */
static int __stm32_uart_fstat(ms_ptr_t ctx, ms_io_file_t *file, ms_stat_t *buf)
{
    privinfo_t *priv = ctx;

    buf->st_size = ms_fifo_size(&priv->rx_fifo);

    return 0;
}

/*
 * Check device readable
 */
static ms_bool_t __stm32_uart_readable_check(ms_ptr_t ctx)
{
    privinfo_t *priv = ctx;

    return ms_fifo_is_empty(&priv->rx_fifo) ? MS_FALSE : MS_TRUE;
}

/*
 * Check device writable
 */
static ms_bool_t __stm32_uart_writable_check(ms_ptr_t ctx)
{
    privinfo_t *priv = ctx;

    return ms_fifo_is_full(&priv->tx_fifo) ? MS_FALSE : MS_TRUE;
}

/*
 * Check device exception
 */
static ms_bool_t __stm32_uart_except_check(ms_ptr_t ctx)
{
    privinfo_t *priv = ctx;

    (void)priv;

    /*
     * NOTE: __stm32_uart_isr ===> __stm32_uart_poll_notify(priv, POLLERR);
     */

    return MS_FALSE;
}

/*
 * Device notify
 */
static int __stm32_uart_poll_notify(privinfo_t *priv, ms_pollevent_t event)
{
    return ms_io_poll_notify_helper(priv->slots, MS_ARRAY_SIZE(priv->slots), event);
}

/*
 * Poll device
 */
static int __stm32_uart_poll(ms_ptr_t ctx, ms_io_file_t *file, ms_pollfd_t *fds, ms_bool_t setup)
{
    privinfo_t *priv = ctx;

    return ms_io_poll_helper(fds, priv->slots, MS_ARRAY_SIZE(priv->slots), setup, ctx,
                             __stm32_uart_readable_check, __stm32_uart_writable_check, __stm32_uart_except_check);
}

/*
 * Receives an amount of data in non blocking mode
 */
static int __stm32_uart_receive_int(privinfo_t *priv)
{
    USART_TypeDef *instance = (USART_TypeDef *)(priv->base);
    ms_uint16_t data = 0;

    if (priv->uart_handler->Init.WordLength == UART_WORDLENGTH_9B) {
        if (priv->uart_handler->Init.Parity == UART_PARITY_NONE) {
            data  = (ms_uint16_t)(instance->RDR & (ms_uint16_t)0x01FF);
            ms_fifo_put(&priv->rx_fifo, (ms_uint8_t *)&data, 2);
        } else {
            data = (ms_uint16_t)(instance->RDR & (ms_uint16_t)0x00FF);
            ms_fifo_put(&priv->rx_fifo, (ms_uint8_t *)&data, 1);
        }
    } else {
        if (priv->uart_handler->Init.Parity == UART_PARITY_NONE) {
            data = (ms_uint8_t)(instance->RDR & (ms_uint8_t)0x00FF);
            ms_fifo_put(&priv->rx_fifo, (ms_uint8_t *)&data, 1);
        } else {
            data = (ms_uint8_t)(instance->RDR & (ms_uint8_t)0x007F);
            ms_fifo_put(&priv->rx_fifo, (ms_uint8_t *)&data, 1);
        }
    }

    (void)ms_semb_post(priv->rx_semb);
    __stm32_uart_poll_notify(priv, POLLIN);

    return MS_ERR_NONE;
}

/*
 * Sends an amount of data in non blocking mode.
 */
static int __stm32_uart_transmit_int(privinfo_t *priv)
{
    USART_TypeDef *instance = (USART_TypeDef *)(priv->base);
    ms_uint16_t data = 0;

    if (priv->uart_handler->Init.WordLength == UART_WORDLENGTH_9B) {
        if (priv->uart_handler->Init.Parity == UART_PARITY_NONE) {
            ms_fifo_get(&priv->tx_fifo, (ms_uint8_t *)&data, 2);
            instance->TDR = (ms_uint32_t)(data & (ms_uint16_t)0x01FF);
        } else {
            ms_fifo_get(&priv->tx_fifo, (ms_uint8_t *)&data, 1);
            instance->TDR = (ms_uint32_t)(data & (ms_uint16_t)0x01FF);
        }
    } else {
        ms_fifo_get(&priv->tx_fifo, (ms_uint8_t *)&data, 1);
        instance->TDR = (ms_uint8_t)(data & (ms_uint8_t)0x00FF);
    }

    if (ms_fifo_is_empty(&priv->tx_fifo) == MS_TRUE) {
        __stm32_uart_int_mode_set(priv, UART_IT_TXE, MS_FALSE);
    }

    (void)ms_semb_post(priv->tx_semb);
    __stm32_uart_poll_notify(priv, POLLOUT);

    return MS_ERR_NONE;
}

/*
 * Wraps up transmission in non blocking mode.
 */
static int __stm32_uart_endtransmit_int(privinfo_t *priv)
{
    /* Disable the UART Transmit Complete Interrupt */
    __stm32_uart_int_mode_set(priv, UART_IT_TC, MS_FALSE);

    ms_printk(MS_PK_ERR, "error: UART Transmit Complete Interrupt [NOT USED]!\n\r");

    return MS_ERR_NONE;
}

/*
 * This function handles UART interrupt request.
 */
static void __stm32_uart_isr(privinfo_t *priv)
{
    UART_HandleTypeDef *uart_handler = priv->uart_handler;
    USART_TypeDef *instance = (USART_TypeDef *)(priv->base);

    uint32_t isrflags   = READ_REG(instance->ISR);
    uint32_t cr1its     = READ_REG(instance->CR1);
    uint32_t cr3its     = READ_REG(instance->CR3);

    /* Clear uart device error_code */
    uart_handler->ErrorCode = HAL_UART_ERROR_NONE;

    /* Disable uart interrupt */
    __stm32_uart_int_set(priv, MS_FALSE);

    /* UART parity error interrupt occurred -------------------------------------*/
    if (((isrflags & USART_ISR_PE) != 0U) && ((cr1its & USART_CR1_PEIE) != 0U))
    {
      __HAL_UART_CLEAR_FLAG(uart_handler, UART_CLEAR_PEF);

      uart_handler->ErrorCode |= HAL_UART_ERROR_PE;
    }

    /* UART frame error interrupt occurred --------------------------------------*/
    if (((isrflags & USART_ISR_FE) != 0U) && ((cr3its & USART_CR3_EIE) != 0U))
    {
      __HAL_UART_CLEAR_FLAG(uart_handler, UART_CLEAR_FEF);

      uart_handler->ErrorCode |= HAL_UART_ERROR_FE;
    }

    /* UART noise error interrupt occurred --------------------------------------*/
    if (((isrflags & USART_ISR_NE) != 0U) && ((cr3its & USART_CR3_EIE) != 0U))
    {
      __HAL_UART_CLEAR_FLAG(uart_handler, UART_CLEAR_NEF);

      uart_handler->ErrorCode |= HAL_UART_ERROR_NE;
    }

    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if (((isrflags & USART_ISR_ORE) != 0U)
        && (((cr1its & USART_CR1_RXNEIE_RXFNEIE) != 0U) ||
            ((cr3its & (USART_CR3_RXFTIE | USART_CR3_EIE)) != 0U)))
    {
      __HAL_UART_CLEAR_FLAG(uart_handler, UART_CLEAR_OREF);

      uart_handler->ErrorCode |= HAL_UART_ERROR_ORE;
    }

    /* UART in mode Receiver ---------------------------------------------------*/
    if (((isrflags & USART_ISR_RXNE_RXFNE) != 0U)
        && (((cr1its & USART_CR1_RXNEIE_RXFNEIE) != 0U)
            || ((cr3its & USART_CR3_RXFTIE) != 0U)))
    {
        __stm32_uart_receive_int(priv);
    }

    /* UART in mode Transmitter ------------------------------------------------*/
    if (((isrflags & USART_ISR_TXE_TXFNF) != 0U)
        && (((cr1its & USART_CR1_TXEIE_TXFNFIE) != 0U)
            || ((cr3its & USART_CR3_TXFTIE) != 0U)))
    {
        __stm32_uart_transmit_int(priv);
    }

    /* UART in mode Transmitter (transmission end) -----------------------------*/
    if (((isrflags & USART_ISR_TC) != 0U) && ((cr1its & USART_CR1_TCIE) != 0U))
    {
        __stm32_uart_endtransmit_int(priv);
    }

    if (uart_handler->ErrorCode != HAL_UART_ERROR_NONE)
    {
        /* Set the UART state ready to be able to start again the process */
        uart_handler->gState = HAL_UART_STATE_READY;
        __stm32_uart_poll_notify(priv, POLLERR);
    }

    /* Enable uart interrupt */
    __stm32_uart_int_set(priv, MS_TRUE);
}

/*
 * Handle UART IRQ
 */
void stm32_uart_irq_handler(ms_uint8_t channel)
{
    stm32_uart_dev_t *dev = &__stm32_uart_devs[channel - 1];

    __stm32_uart_isr(&dev->priv);
}

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_uart_drv_ops = {
        .type   = MS_IO_DRV_TYPE_CHR,
        .open   = __stm32_uart_open,
        .close  = __stm32_uart_close,
        .write  = __stm32_uart_write,
        .read   = __stm32_uart_read,
        .ioctl  = __stm32_uart_ioctl,
        .fstat  = __stm32_uart_fstat,
        .poll   = __stm32_uart_poll,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_uart_drv = {
        .nnode = {
            .name = "stm32_uart",
        },
        .ops = &stm32_uart_drv_ops,
};

/*
 * Register UART device driver
 */
ms_err_t stm32_uart_drv_register(void)
{
    return ms_io_driver_register(&stm32_uart_drv);
}

/*
 * Create UART device file
 */
ms_err_t stm32_uart_dev_create(const char *path, ms_uint8_t channel,
                               ms_uint32_t rx_buf_size, ms_uint32_t tx_buf_size)
{
    stm32_uart_dev_t *uart_dev;
    ms_ptr_t          buffer_ptr;
    ms_err_t          err = MS_ERR_NONE;

    if ((path == MS_NULL) || (channel > __STM32_UART_MAX_CHANNEL) || (channel == 0U)) {
        return MS_ERR;
    }

    uart_dev = &__stm32_uart_devs[channel - 1U];
    bzero(&uart_dev->priv, sizeof(uart_dev->priv));

    if (stm32_uart_get_hw_info(channel, &uart_dev->priv.irq, &uart_dev->priv.uart_handler) != MS_ERR_NONE) {
        return MS_ERR;
    }
    uart_dev->priv.base = (ms_addr_t)uart_dev->priv.uart_handler->Instance;

    rx_buf_size = ms_roundup_pow2_size(rx_buf_size);
    tx_buf_size = ms_roundup_pow2_size(tx_buf_size);

    buffer_ptr = ms_kmalloc(rx_buf_size + tx_buf_size);
    if (buffer_ptr == MS_NULL) {
        return MS_ERR;
    }

    ms_fifo_init(&uart_dev->priv.rx_fifo, (ms_uint8_t *)buffer_ptr, rx_buf_size);
    ms_fifo_init(&uart_dev->priv.tx_fifo, (ms_uint8_t *)buffer_ptr + rx_buf_size, tx_buf_size);

    err = ms_semb_create("uart_rx_semb", MS_FALSE, MS_WAIT_TYPE_PRIO, &uart_dev->priv.rx_semb);
    if (err != MS_ERR_NONE) {
        goto err_rx_semb;
    }

    err = ms_semb_create("uart_tx_semb", MS_FALSE, MS_WAIT_TYPE_PRIO, &uart_dev->priv.tx_semb);
    if (err != MS_ERR_NONE) {
        goto err_tx_semb;
    }

    uart_dev->priv.rx_timeout = MS_TIMEOUT_FOREVER;

    err = ms_io_device_register(&uart_dev->dev, path, "stm32_uart", &uart_dev->priv);
    if (err != MS_ERR_NONE) {
        goto err_dev_reg;
    }

    return MS_ERR_NONE;

err_dev_reg:
    ms_semb_destroy(uart_dev->priv.tx_semb);

err_tx_semb:
    ms_semb_destroy(uart_dev->priv.rx_semb);

err_rx_semb:
    ms_kfree(buffer_ptr);

    return MS_ERR;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
