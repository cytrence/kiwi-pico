/*
 * Copyright (C) 2024 Cytrence Technologies, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/uart.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"
#include "bitmap.h"

// Display settings
#define FRAME_WIDTH             320
#define FRAME_HEIGHT            240
#define VREG_VSEL               VREG_VOLTAGE_1_20
#define DVI_TIMING              dvi_timing_640x480p_60hz

// Frame intervals for smooth display update
#define FRAME_INTERVAL_1        16666
#define FRAME_INTERVAL_2        16667
#define MAX_NUMBER              99999
#define FRAME_COUNT_TARGET      300

struct dvi_inst dvi0;

static uint16_t framebuffer[FRAME_HEIGHT * FRAME_WIDTH];


void core1_main()
{
    // Register IRQs and start DVI scan buffer on core 1
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);

    while (queue_is_empty(&dvi0.q_colour_valid))
    {
        __wfe();
    }

    dvi_start(&dvi0);
    dvi_scanbuf_main_16bpp(&dvi0);
}

void initialize_framebuffer()
{
    // Initialize framebuffer with black color
    for (uint y = 0; y < FRAME_HEIGHT; ++y)
    {
        for (uint x = 0; x < FRAME_WIDTH; ++x)
        {
            framebuffer[y * FRAME_WIDTH + x] = 0x0000;
        }
    }
}

void reset_framebuffer_to_null()
{
    // Reset framebuffer to null values
    for (uint y = 0; y < FRAME_HEIGHT; ++y)
    {
        for (uint x = 0; x < FRAME_WIDTH; ++x)
        {
            framebuffer[y * FRAME_WIDTH + x] = 0x0000;
        }
    }
}

void draw_char(const uint8_t bitmap[16][8], int x, int y)
{
    // Draw a character on the framebuffer at specified position
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            framebuffer[(y + i) * FRAME_WIDTH + (x + j)] = bitmap[i][j] ? 0xFFFF : 0x0000;
        }
    }
}

void clear_digits_area(int num_digits)
{
    // Clear the area in the framebuffer where the digits are displayed
    int total_width = num_digits * 9;
    int x_offset = (FRAME_WIDTH - total_width) / 2;
    int y_offset = (FRAME_HEIGHT - 16) / 2;

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < total_width; j++)
        {
            framebuffer[(y_offset + i) * FRAME_WIDTH + (x_offset + j)] = 0x0000;
        }
    }
}

void update_framebuffer(int number)
{
    // Convert number to string and calculate number of digits
    // The array size is 6 to hold a maximum of 5 digits plus the null terminator
    // It can be increased if a larger number of digits is required
    char str[6];
    snprintf(str, sizeof(str), "%d", number);
    int num_digits = strlen(str);

    clear_digits_area(num_digits);

    int total_width = num_digits * 9;
    int x_offset = (FRAME_WIDTH - total_width) / 2;
    int y_offset = (FRAME_HEIGHT - 16) / 2;

    for (int i = 0; i < num_digits; i++)
    {
        int digit = str[i] - '0';
        draw_char(digit_bitmaps[digit], x_offset + (i * 9), y_offset);
    }
}

void update_framebuffer_sync()
{
    // Synchronize the framebuffer with the DVI output
    for (uint y = 0; y < FRAME_HEIGHT; ++y)
    {
        const uint16_t *scanline = &framebuffer[y * FRAME_WIDTH];
        queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
        while (!queue_try_remove_u32(&dvi0.q_colour_free, &scanline))
        {
            __wfe();
        }
    }
}


int main()
{
    stdio_init_all();

    vreg_set_voltage(VREG_VSEL);
    sleep_ms(10);
    set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

    // UART init
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart0, true);

    // DVI init
    dvi0.timing = &DVI_TIMING;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    multicore_launch_core1(core1_main);

    // my init
    initialize_framebuffer();

    int number = 0;
    uint64_t t0 = to_us_since_boot(get_absolute_time());
    uint64_t next_t = t0 + FRAME_INTERVAL_1;
    uint64_t start_t = t0;

    while (true)
    {
        uint64_t current_t = to_us_since_boot(get_absolute_time());

        if (current_t > next_t)
        {
            if (number == 0)
            {
                reset_framebuffer_to_null();
            }

            update_framebuffer(number);
            update_framebuffer_sync();

            number++;
            if (number == MAX_NUMBER)
            {
                number = 0;
            }
            if (number % FRAME_COUNT_TARGET == 0)
            {
                uint64_t end_t = to_us_since_boot(get_absolute_time());
                printf("Time for %d frames: %llu us\n", FRAME_COUNT_TARGET, end_t - start_t);
                start_t = end_t;
            }

            next_t += (number % 3 == 0) ? FRAME_INTERVAL_1 : FRAME_INTERVAL_2;
        }
        else
        {
            uint64_t wait_t = (next_t - current_t) / 2;
            busy_wait_us_32(wait_t);
        }
    }
    return 0;
}
