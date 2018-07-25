/// Definitions of the fixed locations of certain data in the SPI flash.
/**
 ** The following is the planned flash layout for the flash storage on the
 ** qc15 badge:
 **
 ** The flash chip we're using (S25FS064S) has several options for block sizes.
 ** For simplicity we're going to use uniform sizes. Here are the two density
 ** options offered by the chip:
 **
 ** QC15 LAYOUT
 ** ===========
 **
 ** We're going to use UNIFORM 64 KB BLOCKS.
 **
 ** 0x000000 - reserved (sentinal byte)
 ** 0x010000 - First ID
 ** 0x020000 - main config
 ** 0x030000 - Badge name
 ** 0x040000 - Second ID copy
 ** 0x050000 - Backup config
 ** 0x060000 - Badge name second copy
 ** 0x070000 - Badge names (11 bytes each) (0x070000 - 0x071356)
 ** READ NAMES:
 ** 0x100000 -   0 -  19 (220 bytes) + 16-bit CRC
 ** 0x110000 -  20 -  39 (220 bytes) + 16-bit CRC
 ** ...
 ** 0x300000 - game start
 ** 0x400000 - choice shares
 **
 ** ...
 ** 0x7C0000 - last block
 **
 */

#ifndef FLASH_LAYOUT_H_
#define FLASH_LAYOUT_H_

#define FLASH_SENTINAL_BYTE 0xAB

#define FLASH_ADDR_SENTINAL     0x000000
#define FLASH_ADDR_ID_MAIN      0x010000
#define FLASH_ADDR_CONF_MAIN    0x020000
#define FLASH_ADDR_NAME_MAIN    0x030000
#define FLASH_ADDR_ID_BACKUP    0x040000
#define FLASH_ADDR_CONF_BACKUP  0x050000
#define FLASH_ADDR_NAME_BACKUP  0x060000
#define FLASH_ADDR_BADGE_NAMES  0x070000
#define FLASH_ADDR_PERSON_NAMES 0x100000

#endif /* FLASH_LAYOUT_H_ */
