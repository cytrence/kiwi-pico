/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024, Cytrence Technologies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "common_dvi_pin_configs.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/uart.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

// Display settings
#define FRAME_WIDTH  320
#define FRAME_HEIGHT 240
#define VREG_VSEL    VREG_VOLTAGE_1_20
#define DVI_TIMING   dvi_timing_640x480p_60hz

// Frame intervals for smooth display update
#define FRAME_INTERVAL_1   16666
#define FRAME_INTERVAL_2   16667
#define MAX_NUMBER         99999
#define FRAME_COUNT_TARGET 300

// Error codes
#define ERR_SUCCESS     0
#define ERR_INIT_FAILED -1

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

static void initialize_framebuffer()
{
    // Initialize framebuffer with black color
    memset(framebuffer, 0, sizeof(framebuffer));
}

static void reset_framebuffer_to_0()
{
    // Reset framebuffer to 0
    memset(framebuffer, 0, sizeof(framebuffer));
}

static void draw_char(const uint8_t bitmap[DIGIT_HEIGHT][DIGIT_WIDTH], const int x, const int y)
{
    // Bounds checking
    if (x < 0 || x + DIGIT_WIDTH > FRAME_WIDTH || y < 0 || y + DIGIT_HEIGHT > FRAME_HEIGHT)
    {
        return;
    }

    // Draw a character on the framebuffer at specified position
    for (int i = 0; i < DIGIT_HEIGHT; i++)
    {
        for (int j = 0; j < DIGIT_WIDTH; j++)
        {
            framebuffer[(y + i) * FRAME_WIDTH + (x + j)] = bitmap[i][j] ? 0xFFFF : 0x0000;
        }
    }
}

// Clear the area in the framebuffer where the digits are displayed
static void clear_digits_area(const int num_digits)
{
    if (num_digits <= 0)
        return;

    const int total_width = num_digits * (DIGIT_WIDTH + DIGIT_SPACING);
    const int x_offset = (FRAME_WIDTH - total_width) / 2;
    const int y_offset = (FRAME_HEIGHT - DIGIT_HEIGHT) / 2;

    // Bounds checking
    if (x_offset < 0 || y_offset < 0)
        return;

    for (int i = 0; i < DIGIT_HEIGHT; i++)
    {
        memset(&framebuffer[(y_offset + i) * FRAME_WIDTH + x_offset], 0, total_width * sizeof(uint16_t));
    }
}

static void update_framebuffer(const int number)
{
    // Convert number to string and calculate number of digits
    // The array size is 6 to hold a maximum of 5 digits plus the null terminator
    // It can be increased if a larger number of digits is required
    char str[6];
    const int num_digits = snprintf(str, sizeof(str), "%d", number);
    if (num_digits < 0 || num_digits >= sizeof(str))
        return;

    clear_digits_area(num_digits);

    const int total_width = num_digits * (DIGIT_WIDTH + DIGIT_SPACING);
    const int x_offset = (FRAME_WIDTH - total_width) / 2;
    const int y_offset = (FRAME_HEIGHT - DIGIT_HEIGHT) / 2;

    for (int i = 0; i < num_digits; i++)
    {
        const int digit = str[i] - '0';
        if (digit >= 0 && digit <= 9)
        {
            draw_char(digit_bitmaps[digit], x_offset + (i * (DIGIT_WIDTH + DIGIT_SPACING)), y_offset);
        }
    }
}

static void update_framebuffer_sync(void)
{
    // Synchronize the framebuffer with the DVI output
    for (uint y = 0; y < FRAME_HEIGHT; ++y)
    {
        const uint16_t* scanline = &framebuffer[y * FRAME_WIDTH];
        queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
        while (!queue_try_remove_u32(&dvi0.q_colour_free, &scanline))
        {
            __wfe();
        }
    }
}

static int initialize_hardware(void)
{
    // Initialize voltage regulator
    vreg_set_voltage(VREG_VSEL);
    sleep_ms(10);

    // Set system clock
    if (!set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true))
    {
        return ERR_INIT_FAILED;
    }

    // Initialize UART
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart0, true);

    // Initialize DVI
    dvi0.timing = &DVI_TIMING;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    return ERR_SUCCESS;
}

int main(void)
{
    stdio_init_all();

    if (initialize_hardware() != ERR_SUCCESS)
    {
        printf("Hardware initialization failed\n");
        return ERR_INIT_FAILED;
    }

    multicore_launch_core1(core1_main);
    initialize_framebuffer();

    int number = 0;
    const uint64_t t0 = to_us_since_boot(get_absolute_time());
    uint64_t next_t = t0 + FRAME_INTERVAL_1;
    uint64_t start_t = t0;

    while (true)
    {
        const uint64_t current_t = to_us_since_boot(get_absolute_time());

        if (current_t > next_t)
        {
            if (number == 0)
            {
                reset_framebuffer_to_0();
            }

            update_framebuffer(number);
            update_framebuffer_sync();

            number++;
            if (number >= MAX_NUMBER)
            {
                number = 0;
            }

            if (number % FRAME_COUNT_TARGET == 0)
            {
                const uint64_t end_t = to_us_since_boot(get_absolute_time());
                printf("Time for %d frames: %llu us\n", FRAME_COUNT_TARGET, end_t - start_t);
                start_t = end_t;
            }

            next_t += (number % 3 == 0) ? FRAME_INTERVAL_1 : FRAME_INTERVAL_2;
        }
        else
        {
            const uint64_t wait_t = (next_t - current_t) / 2;
            busy_wait_us_32(wait_t);
        }
    }

    return ERR_SUCCESS;
}
