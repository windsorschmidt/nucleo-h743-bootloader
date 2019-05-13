#include <stdio.h>

#include "main.h"
#include "serial.h"
#include "stm32ld.h"
#include "type.h"

#define STM32_RETRY_COUNT 4

#define STM32_EXPECT(expected)                                                 \
    if (stm32h_read_byte() != expected)                                        \
        return STM32_COMM_ERROR;

#define STM32_READ_AND_CHECK(x)                                                \
    if ((x = stm32h_read_byte()) == -1)                                        \
        return STM32_COMM_ERROR;

static void stm32h_send_command(u8 cmd)
{
    ser_write_byte(cmd);
    ser_write_byte(~cmd);
}

static int stm32h_read_byte() { return ser_read_byte(); }

static int stm32h_send_packet_with_checksum(u8 *packet, u32 len)
{
    u8 chksum = 0;
    u32 i;

    for (i = 0; i < len; i++)
        chksum ^= packet[i];

    ser_write(packet, len);
    ser_write_byte(chksum);

    return STM32_OK;
}

static int stm32h_send_address(u32 address)
{
    u8 addr_buf[4];

    addr_buf[0] = address >> 24;
    addr_buf[1] = (address >> 16) & 0xFF;
    addr_buf[2] = (address >> 8) & 0xFF;
    addr_buf[3] = address & 0xFF;

    return stm32h_send_packet_with_checksum(addr_buf, 4);
}

static int stm32h_connect_to_bl()
{
    int res;

    /* first flush read buffer */
    ser_set_timeout_ms(1);
    while (stm32h_read_byte() != -1) {
    }
    ser_set_timeout_ms(1000);

    ser_write_byte(STM32_CMD_INIT);
    ser_write_byte(STM32_CMD_INIT);
    res = stm32h_read_byte();

    if (res == -1) {
        printf("connect timeout\n");
    }

    return res == STM32_COMM_ACK || res == STM32_COMM_NACK ? STM32_OK
                                                           : STM32_INIT_ERROR;
}

int stm32_init(void)
{
    ser_setup();

    return stm32h_connect_to_bl();
}

int stm32_get_version(u8 *major, u8 *minor)
{
    u8 i, version = 0;
    int temp, total;

    stm32h_send_command(STM32_CMD_GET_COMMAND);
    STM32_EXPECT(STM32_COMM_ACK);
    STM32_READ_AND_CHECK(total);

    for (i = 0; i < total + 1; i++) {
        STM32_READ_AND_CHECK(temp);
        if (i == 0)
            version = (u8)temp;
    }

    *major = version >> 4;
    *minor = version & 0x0F;
    STM32_EXPECT(STM32_COMM_ACK);

    return STM32_OK;
}

int stm32_get_chip_id(u16 *version)
{
    int vh, vl;

    stm32h_send_command(STM32_CMD_GET_ID);
    STM32_EXPECT(STM32_COMM_ACK);
    STM32_EXPECT(1);
    STM32_READ_AND_CHECK(vh);
    STM32_READ_AND_CHECK(vl);
    STM32_EXPECT(STM32_COMM_ACK);
    *version = ((u16)vh << 8) | (u16)vl;
    return STM32_OK;
}

int stm32_write_unprotect()
{
    stm32h_send_command(STM32_CMD_WRITE_UNPROTECT);
    STM32_EXPECT(STM32_COMM_ACK);
    STM32_EXPECT(STM32_COMM_ACK);

    /* target resets automatically after unprotect */

    HAL_Delay(100);
    return stm32h_connect_to_bl();
}

int stm32_erase_flash()
{
    stm32h_send_command(STM32_CMD_ERASE_FLASH);
    STM32_EXPECT(STM32_COMM_ACK);
    ser_write_byte(0xFF);
    ser_write_byte(0x00);
    STM32_EXPECT(STM32_COMM_ACK);
    return STM32_OK;
}

int stm32_extended_erase_flash()
{
    stm32h_send_command(STM32_CMD_EXTENDED_ERASE_FLASH);
    STM32_EXPECT(STM32_COMM_ACK);
    ser_write_byte(0xFF);
    ser_write_byte(0xFF);
    ser_write_byte(0x00);
    ser_set_timeout_ms(SER_ERASE_TIMEOUT);
    STM32_EXPECT(STM32_COMM_ACK);
    return STM32_OK;
}

int stm32_write_flash(p_read_data read_data_func, p_progress progress_func)
{
    u32 wrote = 0;
    u8 data[STM32_WRITE_BUFSIZE + 1];
    u32 datalen, address = STM32_FLASH_START_ADDRESS;

    while (1) {
        if ((datalen = read_data_func(data + 1, STM32_WRITE_BUFSIZE)) == 0) {
            break;
        }
        data[0] = (u8)(datalen - 1);

        stm32h_send_command(STM32_CMD_WRITE_FLASH);
        STM32_EXPECT(STM32_COMM_ACK);

        stm32h_send_address(address);
        STM32_EXPECT(STM32_COMM_ACK);

        stm32h_send_packet_with_checksum(data, datalen + 1);
        STM32_EXPECT(STM32_COMM_ACK);

        wrote += datalen;
        if (progress_func) {
            progress_func(wrote);
        }

        address += datalen;
    }
    return STM32_OK;
}

int stm32_go_command(void)
{
    u32 address = STM32_FLASH_START_ADDRESS;

    stm32h_send_command(STM32_CMD_GO);
    STM32_EXPECT(STM32_COMM_ACK);

    stm32h_send_address(address);
    STM32_EXPECT(STM32_COMM_ACK);

    /* target should begin running it sends one ACK */

    return STM32_OK;
}
