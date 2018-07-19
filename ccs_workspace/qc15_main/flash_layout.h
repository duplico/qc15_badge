/// Definitions of the fixed locations of certain data in the SPI flash.
/**
 ** The following is the planned flash layout for the flash storage on the
 ** qc15 badge:
 **
 ** The flash chip we're using (S25FS064S) has several options for block sizes.
 ** For simplicity we're going to use uniform sizes. Here are the two density
 ** options offered by the chip:
 **
 ** UNIFORM 64 KB BLOCKS
 ** ====================
 **
 ** SA00: 0x00000000-0x0000FFFF
 ** SA01: 0x00010000-0x0001FFFF
 ** SA02: 0x00020000-0x0002FFFF
 ** ...
 ** SA127 0x007F0000-0x007FFFFF
 **
 ** UNIFORM 256 KB BLOCKS
 ** =====================
 **
 ** SA00: 0x00000000-0x0003FFFF
 ** SA01: 0x00040000-0x0007FFFF
 ** SA02: 0x00080000-0x000BFFFF
 ** ...
 ** SA32: 0x007C0000-0x007FFFFF
 **
 ** 0x00000000 - reserved
 **
 ** QC15 LAYOUT
 ** ===========
 **
 ** We're going to use UNIFORM 64 KB BLOCKS.
 **
 ** 0x000000 - reserved
 ** 0x010000 - First ID
 ** 0x020000 - main config
 ** 0x030000 - Badge name
 ** 0x040000 - Second ID copy
 ** 0x050000 - Backup config
 ** 0x060000 - Badge name second copy
 ** 0x070000 - Badge names (11 bytes each) (0x070000 - 0x071356)
 ** READ NAMES:
 ** 0x100000 -   0 -  24 (250 bytes)
 ** 0x110000 -  25 -  49 (250 bytes)
 ** 0x120000 -  50 -  74 (250 bytes)
 ** 0x130000 -  75 -  99 (250 bytes)
 ** 0x140000 - 100 - 124 (250 bytes)
 ** 0x150000 - 125 - 149 (250 bytes)
 ** 0x160000 - 150 - 174 (250 bytes)
 ** 0x170000 - 175 - 199 (250 bytes)
 ** 0x180000 - 200 - 224 (250 bytes)
 ** 0x190000 - 225 - 249 (250 bytes)
 ** 0x1a0000 - 250 - 274 (250 bytes)
 ** 0x1b0000 - 275 - 299 (250 bytes)
 ** 0x1c0000 - 300 - 324 (250 bytes)
 ** 0x1d0000 - 325 - 349 (250 bytes)
 ** 0x1e0000 - 350 - 374 (250 bytes)
 ** 0x1f0000 - 375 - 399 (250 bytes)
 ** 0x200000 - 400 - 424 (250 bytes)
 ** 0x210000 - 425 - 449 (250 bytes)
 ** 0x220000 - 450 - 474 (250 bytes)
 ** ...
 ** 0x300000 - game start
 ** 0x400000 - choice shares
 **
 ** ...
 ** 0x7C0000 - last block
 **
 ** \file flash_layout.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan <duplico@dupli.co> @duplico. MIT License.
 */

#ifndef FLASH_LAYOUT_H_
#define FLASH_LAYOUT_H_

#define FLASH_ADDR_ID_MAIN      0x010000
#define FLASH_ADDR_CONF_MAIN    0x020000
#define FLASH_ADDR_NAME_MAIN    0x030000
#define FLASH_ADDR_ID_BACKUP    0x040000
#define FLASH_ADDR_CONF_BACKUP  0x050000
#define FLASH_ADDR_NAME_BACKUP  0x060000
#define FLASH_ADDR_BADGE_NAMES  0x070000
#define FLASH_ADDR_PERSON_NAMES 0x100000

#endif /* FLASH_LAYOUT_H_ */
