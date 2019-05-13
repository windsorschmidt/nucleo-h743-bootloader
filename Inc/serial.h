#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "type.h"

#define SER_ERASE_TIMEOUT 0xFFFFFFFF

#define RX_BUFFER_SIZE 32
void ser_uart_isr(void);

void ser_setup(void);
int ser_read_byte(void);
u32 ser_write(const u8 *src, u32 size );
u32 ser_write_byte(u8 data );
void ser_set_timeout_ms(u32 timeout );

#endif
