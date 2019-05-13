#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "payload.h"
#include "stm32ld.h"

#define BL_VERSION_MAJOR 2
#define BL_VERSION_MINOR 1
#define BL_MKVER(major, minor) ((major)*256 + (minor))
#define BL_MINVERSION BL_MKVER(BL_VERSION_MAJOR, BL_VERSION_MINOR)

/* see STM32 reference manuals for part-specific IDs */
static const uint16_t SUPPORTED_CHIP_IDS[] = {0x450};

static u32 read_payload(u8 *dst, u32 len)
{
    /* read from struct in payload.h */
    static u32 nread = 0;
    static char *pptr = (char *) payload;

    u32 rem = sizeof(payload) - nread;
    u32 n = (rem < len) ? rem : len;
    memcpy(dst, pptr + nread, n);
    nread += n;
    return n;
}

static void write_progress(u32 wrote)
{
    static u32 w = 0;
    w += wrote;
    printf("wrote %07lu bytes (%02lu%%)\n", wrote,
           ((w * 100) / sizeof(payload)) / 10);
}

static void flash_failed(void)
{
    printf("flash failed\n");
    while (1) {
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin); /* LD3 = red LED */
        HAL_Delay(200);
    }
}

void app_main(void)
{
    u8 not_flashing = 0;
    u8 send_go_command = 1;
    u8 minor, major;
    u16 version;

    if (stm32_init() != STM32_OK) {
        printf("unable to connect to bootloader\n");
        flash_failed();
    }

    if (stm32_get_version(&major, &minor) != STM32_OK) {
        printf("unable to get bootloader version\n");
        flash_failed();
    } else {
        printf("got bootloader version: %d.%d\n", major, minor);
        if (BL_MKVER(major, minor) < BL_MINVERSION) {
            printf("unsupported bootloader version\n");
            flash_failed();
        }
    }

    if (stm32_get_chip_id(&version) != STM32_OK) {
        printf("unable to get chip ID\n");
        flash_failed();
    } else {
        const uint16_t *chip_ids = SUPPORTED_CHIP_IDS;
        printf("got chip ID: %04X\n", version);
        while (*chip_ids != 0) {
            if (*chip_ids == version) {
                break;
            }
            chip_ids++;
        }
        if (*chip_ids == 0) {
            printf("unsupported chip ID\n");
            flash_failed();
        }
    }

    if (not_flashing == 0) {
        if (stm32_write_unprotect() != STM32_OK) {
            printf("unable to execute write unprotect\n");
            flash_failed();
        } else {
            printf("cleared write protection\n");
        }
        if (major == 3) {
            printf("starting extended erase of FLASH memory\n");
            if (stm32_extended_erase_flash() != STM32_OK) {
                printf("unable to extended erase chip\n");
                flash_failed();
            } else {
                printf("completed extended erase\n");
            }
        } else {
            if (stm32_erase_flash() != STM32_OK) {
                printf("unable to erase chip\n");
                flash_failed();
            } else {
                printf("completed erase\n");
            }
        }
        printf("programming flash\n");
        if (stm32_write_flash(read_payload, write_progress) != STM32_OK) {
            printf("unable to program FLASH memory\n");
            flash_failed();
        } else {
            printf("flashing complete\n");
        }
    } else {
        printf("skipping flash operation\n");
    }

    if (send_go_command == 1) {
        printf("sending GO command\n");
        if (stm32_go_command() != STM32_OK) {
            printf("unable to run GO command\n");
            flash_failed();
        }
    }

    printf("all done\n");

    while (1) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); /* LD2 =blue LED */
        HAL_Delay(200);
    }
}
