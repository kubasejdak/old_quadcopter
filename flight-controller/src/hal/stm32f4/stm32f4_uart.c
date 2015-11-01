//---------------------------------------------------------------------------------------------------------------
//
// Filename   : stm32f4_uart.c
// Author     : Kuba Sejdak
// Created on : 11.08.2015
//
// This file is a part of SkyViper project.
//
// %LICENSE%
//
//---------------------------------------------------------------------------------------------------------------

#include "stm32f4_uart.h"
#include "stm32f4_rcc.h"
#include "stm32f4_gpio_functions.h"
#include "CMSIS/stm32f4xx.h"

typedef USART_TypeDef UART_t;

//---------------------------------------------------------------------------------------------------------------
// HELPER FUNCTIONS
//---------------------------------------------------------------------------------------------------------------

int stm32f4_uartToPinFunction(UARTHandle_t *handle)
{
    switch(handle->device) {
    case STM32F4_USART_1:   return GPIO_AF_USART1;
    case STM32F4_USART_2:   return GPIO_AF_USART2;
    case STM32F4_USART_3:   return GPIO_AF_USART3;
    case STM32F4_UART_4:    return GPIO_AF_UART4;
    case STM32F4_UART_5:    return GPIO_AF_UART5;
    case STM32F4_USART_6:   return GPIO_AF_USART6;
    }

    return -1;
}

UART_t *stm32f4_uartGetRegisters(UARTDevice_t device)
{
    switch(device) {
    case STM32F4_USART_1:   return USART1;
    case STM32F4_USART_2:   return USART2;
    case STM32F4_USART_3:   return USART3;
    case STM32F4_UART_4:    return UART4;
    case STM32F4_UART_5:    return UART5;
    case STM32F4_USART_6:   return USART6;
    }

    return 0;
}

uint32_t stm32f4_uartGetDataBitsValue(UARTDataBits_t data_bits)
{
    switch(data_bits) {
    case UART_DATA_BITS_8:  return 0x00000000;
    case UART_DATA_BITS_9:  return 0x00001000;
    default:                break;
    }

    return 0;
}

uint32_t stm32f4_uartGetStopBitsValue(UARTStopBits_t stop_bits)
{
    switch(stop_bits) {
    case UART_STOP_BITS_0_5:    return 0x00001000;
    case UART_STOP_BITS_1:      return 0x00000000;
    case UART_STOP_BITS_1_5:    return 0x00003000;
    case UART_STOP_BITS_2:      return 0x00002000;
    }

    return 0;
}

uint32_t stm32f4_uartGetPartityValue(UARTPartity_t partity)
{
    switch(partity) {
    case UART_PARTITY_NONE:     return 0x00000000;
    case UART_PARTITY_EVEN:     return 0x00000400;
    case UART_PARTITY_ODD:      return 0x00000600;
    }

    return 0;
}

uint32_t stm32f4_uartGetFlowControlValue(UARTFlowControl_t flow_control)
{
    switch(flow_control) {
    case UART_FLOW_CONTROL_NONE:        return 0x00000000;
    case UART_FLOW_CONTROL_RTS_CTS:     return 0x00000300;
    default:                            break;
    }

    return 0;
}

uint32_t stm32f4_uartGetDirectionValue(UARTDirection_t direction)
{
    switch(direction) {
    case UART_DIRECTION_READ:           return 0x00000004;
    case UART_DIRECTION_WRITE:          return 0x00000008;
    case UART_DIRECTION_BIDIRECTIONAL:  return 0x0000000C;
    }

    return 0;
}

void stm32f4_uartEnableClock(UARTDevice_t device, bool value)
{
    switch(device) {
    case STM32F4_USART_1:   stm32f4_rccEnablePeripheralClockAPB2(RCC_APB2_PERIPHERAL_USART1, value); break;
    case STM32F4_USART_2:   stm32f4_rccEnablePeripheralClockAPB1(RCC_APB1_PERIPHERAL_USART2, value); break;
    case STM32F4_USART_3:   stm32f4_rccEnablePeripheralClockAPB1(RCC_APB1_PERIPHERAL_USART3, value); break;
    case STM32F4_UART_4:    stm32f4_rccEnablePeripheralClockAPB1(RCC_APB1_PERIPHERAL_UART4, value);  break;
    case STM32F4_UART_5:    stm32f4_rccEnablePeripheralClockAPB1(RCC_APB1_PERIPHERAL_UART5, value);  break;
    case STM32F4_USART_6:   stm32f4_rccEnablePeripheralClockAPB2(RCC_APB2_PERIPHERAL_USART6, value); break;
    }
}

//---------------------------------------------------------------------------------------------------------------
// INTERFACE FUNCTIONS
//---------------------------------------------------------------------------------------------------------------

bool stm32f4_uartInit(UARTHandle_t *handle, STM32F4_UARTConfig_t config)
{
    if(config.general_config.protocol.flow_control != UART_FLOW_CONTROL_NONE) {
        // Hardware flow control is available only for USART1, USART2, USART3 and USART6.
        if(handle->device == STM32F4_UART_4 || handle->device == STM32F4_UART_5)
            return false;
    }

    if(config.general_config.mode == UART_MODE_SYNCHRONOUS && (handle->device == STM32F4_UART_4 || handle->device == STM32F4_UART_5))
        return false;

    if(config.general_config.mode == UART_MODE_SYNCHRONOUS)
        stm32f4_uartClockInit(handle, config.clock_config);

    stm32f4_uartEnableClock(handle->device, true);

    UART_t *uart = stm32f4_uartGetRegisters(handle->device);

    uart->CR1 |= stm32f4_uartGetDataBitsValue(config.general_config.protocol.data_bits);
    uart->CR2 |= stm32f4_uartGetStopBitsValue(config.general_config.protocol.stop_bits);
    uart->CR1 |= stm32f4_uartGetPartityValue(config.general_config.protocol.parity);
    uart->CR3 |= stm32f4_uartGetFlowControlValue(config.general_config.protocol.flow_control);
    uart->CR1 |= stm32f4_uartGetDirectionValue(config.general_config.direction);

    // Configure baud rate.
    STM32F4_ClockFrequencies_t clocks_frequencies;
    stm32f4_rccGetClocksFrequencies(&clocks_frequencies);

    uint32_t uart_freq = 0;
    if(handle->device == STM32F4_USART_1 || handle->device == STM32F4_USART_6)
        uart_freq = clocks_frequencies.pclk2_frequency_hz;
    else
        uart_freq = clocks_frequencies.pclk1_frequency_hz;

    // Determine the integer part.
    uint32_t integer_divider = 0;
    if((uart->CR1 & USART_CR1_OVER8) != 0)
        integer_divider = (25 * uart_freq) / (2 * config.general_config.protocol.baud_rate);
    else
        integer_divider = (25 * uart_freq) / (4 * config.general_config.protocol.baud_rate);

    uint32_t baud_rate_value = (integer_divider / 100) << 4;

    // Determine the fractional part.
    uint32_t fractional_divider = integer_divider - (100 * (baud_rate_value >> 4));
    if((uart->CR1 & USART_CR1_OVER8) != 0)
        baud_rate_value |= ((((fractional_divider * 8) + 50) / 100)) & 0x07;
    else
        baud_rate_value |= ((((fractional_divider * 16) + 50) / 100)) & 0x0F;

    uart->BRR = baud_rate_value;

    uart_activate(handle);
    return true;
}

void stm32f4_uartClockInit(UARTHandle_t *handle, STM32F4_UARTClockConfig_t clock_config)
{
    UART_t *uart = stm32f4_uartGetRegisters(handle->device);

    uart->CR2 |= clock_config.enabled;
    uart->CR2 |= clock_config.polarity;
    uart->CR2 |= clock_config.phase;
    uart->CR2 |= clock_config.last_bit;
}

void uart_activate(UARTHandle_t *handle)
{
    UART_t *uart = stm32f4_uartGetRegisters(handle->device);
    uart->CR1 |= USART_CR1_UE;
}

void uart_deactivate(UARTHandle_t *handle)
{
    UART_t *uart = stm32f4_uartGetRegisters(handle->device);
    uart->CR1 &= ~USART_CR1_UE;
}

void uart_send(UARTHandle_t *handle, uint16_t data)
{
    UART_t *uart = stm32f4_uartGetRegisters(handle->device);

    // Wait until previous transfer completes.
    while(!(uart->SR & USART_SR_TXE));

    uart->DR = data & 0x1ff;
}

uint16_t uart_receive(UARTHandle_t *handle)
{
    UART_t *uart = stm32f4_uartGetRegisters(handle->device);
    return (uart->DR & 0x01ff);
}