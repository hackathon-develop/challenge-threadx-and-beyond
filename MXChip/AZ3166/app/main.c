/* 
 * Copyright (c) Microsoft
 * Copyright (c) 2024 Eclipse Foundation
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors: 
 *     Microsoft         - Initial version
 *     Frédéric Desbiens - 2024 version.
 */

#include "hackathon.h"
#include <stdio.h>

#include "tx_api.h"

#include "board_init.h"
#include "cmsis_utils.h"

#define ECLIPSETX_THREAD_STACK_SIZE 4096
#define ECLIPSETX_THREAD_PRIORITY   4

void tx_application_define(void* first_unused_memory)
{
    systick_interval_set(TX_TIMER_TICKS_PER_SECOND);
    UINT status;

    status = tx_thread_create(&xxx_network_thread,
        "Eclipse ThreadX network Thread",
        xxx_network_thread_entry,
        0,
        xxx_network_thread_stack,
        ECLIPSETX_THREAD_STACK_SIZE,
        ECLIPSETX_THREAD_PRIORITY,
        ECLIPSETX_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: network thread creation failed\r\n");
    }

    status = tx_thread_create(&xxx_sensor_thread,
        "Eclipse ThreadX sensor Thread",
        xxx_sensor_thread_entry,
        0,
        xxx_sensor_thread_stack,
        ECLIPSETX_THREAD_STACK_SIZE,
        ECLIPSETX_THREAD_PRIORITY,
        ECLIPSETX_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: sensor thread creation failed\r\n");
    }
    
    status = tx_thread_create(&xxx_display_thread,
        "Eclipse ThreadX display Thread",
        xxx_display_thread_entry,
        0,
        xxx_display_thread_stack,
        ECLIPSETX_THREAD_STACK_SIZE,
        ECLIPSETX_THREAD_PRIORITY,
        ECLIPSETX_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: display thread creation failed\r\n");
    }
}

int main(void)
{
    // Initialize the board
    board_init();

    // Enter the ThreadX kernel
    tx_kernel_enter();

    return 0;
}
