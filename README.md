## About

Flash STM32 firmware via UART from another STM32. Based on [stm32ld](https://github.com/jsnyder/stm32ld).

## Hardware Setup

Two [NUCLEO-H743ZI](https://www.st.com/en/evaluation-tools/nucleo-h743zi.html) boards with USART2 interconnected (swapping RXD & TXD). Header pins on CN12 carry RXD/PA3 @ pin 37, and TXD/PA2 at pin 35. Also connect GND between boards (available at CN12 pin 20).

On the target board, assert BOOT0 by bridging CN11 pins 7 and 5.

To ensure only the host board's STLINK and VCP are enumerated by the development workstation, power the target board from a dedicated USB charger.

## Running

Using a terminal program, connect to the STLINK virtual com port (VCP) on the development workstation (e.g. /dev/ttyUSB0), with port settings 115200,8N1.

Build and flash firmware to the host board:

```make flash```

After flashing, the host board firmware will immediately erase and flash the target board. Status messages are sent to the VCP on the development workstation.

After flashing is complete, the target board will restart and begin executing the firmware sent by the host board. LEDs LD2 (blue) and LD3 (red) blink as the target runs. On the host, a blinking blue LED indicates success (flashing red indicates failure).

## References

ST reference documents: [UM2407](https://www.st.com/content/ccc/resource/technical/document/user_manual/group1/95/1a/9a/89/87/6a/45/70/DM00499160/files/DM00499160.pdf/jcr:content/translations/en.DM00499160.pdf), [RM0433](https://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/c9/a3/76/fa/55/46/45/fa/DM00314099/files/DM00314099.pdf/jcr:content/translations/en.DM00314099.pdf), [AN3155](https://www.st.com/content/ccc/resource/technical/document/application_note/51/5f/03/1e/bd/9b/45/be/CD00264342.pdf/files/CD00264342.pdf/jcr:content/translations/en.CD00264342.pdf).

The UART connection between boards looks like this:

![Board connections](board-connections.jpg?raw=true "Board connections")
