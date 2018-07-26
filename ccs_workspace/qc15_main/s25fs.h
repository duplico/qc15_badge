/// Header for the minimal driver for the S25FS064S SPI flash module.
/**
 ** \file s25fs.h
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */

#ifndef S25FS_H_
#define S25FS_H_

void s25fs_init();
void s25fs_init_io();
void s25fs_hold_io();
uint8_t s25fs_post1();
uint8_t s25fs_post2();
void s25fs_erase_block_64kb(uint32_t address);

void s25fs_wr_en();
void s25fs_wr_dis();
void s25fs_block_while_wip();
void s25fs_read_data(uint8_t* buffer, uint32_t address, uint32_t len_bytes);
void s25fs_write_data(uint32_t address, uint8_t* buffer, uint32_t len_bytes);

#endif /* S25FS_H_ */
