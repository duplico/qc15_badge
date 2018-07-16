/// The implementation of the multi-badge codebreaking game.
/**
 *  \file codes.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */
#include <stdint.h>

#include <msp430.h>

#include "qc15.h"

uint8_t my_segment_ids[6] = {0};

