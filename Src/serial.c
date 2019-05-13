#include <stdio.h>

#include "app.h"
#include "main.h"
#include "serial.h"

static u32 ser_timeout = 1000;

extern UART_HandleTypeDef huart3; // UART3: STLINK virtual serial port

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return 0;
}

int __io_getchar(void)
{
    int ch;
    HAL_UART_Receive(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

extern UART_HandleTypeDef huart2; // UART2: bootloader target

void ser_setup(void) { __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE); }

char rx_buffer[RX_BUFFER_SIZE];
int rx_index = 0;
int rx_tail = 0;

void ser_uart_isr(void)
{
    if (USART2->ISR & USART_ISR_RXNE_RXFNE) {
        char rx = (char)(USART2->RDR & 0xFF);
        rx_buffer[rx_index++] = rx;
        if (rx_index == RX_BUFFER_SIZE) {
            rx_index = 0;
        }
    }
    __HAL_UART_SEND_REQ(&huart2, UART_RXDATA_FLUSH_REQUEST);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
}

int ser_read_byte(void)
{
    uint32_t t = HAL_GetTick();
    while (rx_index == rx_tail) {
        if ((HAL_GetTick() - t) > ser_timeout) {
            return -1; /* timeout */
        }
    }

    u8 b = rx_buffer[rx_tail++];

    if (rx_tail == RX_BUFFER_SIZE) {
        rx_tail = 0;
    }

    return b;
}

u32 ser_write(const u8 *src, u32 size)
{
    HAL_StatusTypeDef rval;
    rval = HAL_UART_Transmit(&huart2, (unsigned char *)src, size, ser_timeout);
    if (rval == HAL_TIMEOUT) {
        printf("write timeout\n");
        return 0;
    }

    return size;
}

u32 ser_write_byte(u8 data)
{
    HAL_StatusTypeDef rval;
    rval = HAL_UART_Transmit(&huart2, &data, 1, ser_timeout);
    if (rval == HAL_TIMEOUT) {
        printf("byte write timeout\n");
        return 0;
    }

    return 1;
}

void ser_set_timeout_ms(u32 timeout) { ser_timeout = timeout; }
