/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gattUpdateTemp(uint8_t currentTemp);
void gattUpdateMode(uint8_t mode);
void gattUpdateSetpoint(uint8_t setpoint);

/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,

    IDX_CHAR_SET_TEMP,
    IDX_CHAR_VAL_SET_TEMP,

    IDX_CHAR_MODE,
    IDX_CHAR_VAL_MODE,

    HRS_IDX_NB,
};
