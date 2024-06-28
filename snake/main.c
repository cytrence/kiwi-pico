#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"
#include "tmds_encode.h"
#include "tusb.h"
#include "bsp/board.h"
#include "hardware/timer.h"
#include "hardware/uart.h"

// Display settings
#define FRAME_WIDTH             320
#define FRAME_HEIGHT            240
#define VREG_VSEL               VREG_VOLTAGE_1_20
#define DVI_TIMING              dvi_timing_640x480p_60hz

// Colors and block sizes
#define BLOCK_SIZE              8      // Multiple of 8
#define BORDER_SIZE             BLOCK_SIZE
#define BORDER_COLOR            0x3bbb // Blue color in RGB565
#define BACKGROUND_COLOR        0x9f53 // Light green color in RGB565
#define SNAKE_COLOR             0x1ca3 // Green color in RGB565
#define FOOD_COLOR              0xfaca // Red color in RGB565

// Snake game settings
#define SNAKE_MOVE_INTERVAL_MS  250
#define INITIAL_SNAKE_LENGTH    5
#define INITIAL_SNAKE_X         10
#define INITIAL_SNAKE_Y         5
#define INITIAL_FOOD_X          10
#define INITIAL_FOOD_Y          10
#define INITIAL_SNAKE_DIRECTION 1  // 0 = up, 1 = right, 2 = down, 3 = left

// DVI instance
struct dvi_inst dvi0;

// Framebuffer
static uint16_t framebuffer[FRAME_HEIGHT * FRAME_WIDTH];

// Snake and game state
int snake_x[100];   // Max length of snake is 100
int snake_y[100];
int snake_length;
int snake_direction;
int food_x;
int food_y;
bool update_snake = false;
bool move_snake_flag = false;

bool repeating_timer_callback(struct repeating_timer *t)
{
    move_snake_flag = true;
    return true;
}

void core1_main()
{
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    while (queue_is_empty(&dvi0.q_colour_valid))
        __wfe();
    dvi_start(&dvi0);
    dvi_scanbuf_main_16bpp(&dvi0);
}

void initialize_framebuffer()
{
    for (uint y = 0; y < FRAME_HEIGHT; ++y)
    {
        for (uint x = 0; x < FRAME_WIDTH; ++x)
        {
            framebuffer[y * FRAME_WIDTH + x] = BACKGROUND_COLOR;
        }
    }
}

void draw_border()
{
    // Top border
    for (uint y = 0; y < BORDER_SIZE; ++y)
    {
        for (uint x = 0; x < FRAME_WIDTH; ++x)
        {
            framebuffer[y * FRAME_WIDTH + x] = BORDER_COLOR;
        }
    }

    // Bottom border
    for (uint y = FRAME_HEIGHT - BORDER_SIZE; y < FRAME_HEIGHT; ++y)
    {
        for (uint x = 0; x < FRAME_WIDTH; ++x)
        {
            framebuffer[y * FRAME_WIDTH + x] = BORDER_COLOR;
        }
    }

    // Left border
    for (uint y = BORDER_SIZE; y < FRAME_HEIGHT - BORDER_SIZE; ++y)
    {
        for (uint x = 0; x < BORDER_SIZE; ++x)
        {
            framebuffer[y * FRAME_WIDTH + x] = BORDER_COLOR;
        }
    }

    // Right border
    for (uint y = BORDER_SIZE; y < FRAME_HEIGHT - BORDER_SIZE; ++y)
    {
        for (uint x = FRAME_WIDTH - BORDER_SIZE; x < FRAME_WIDTH; ++x)
        {
            framebuffer[y * FRAME_WIDTH + x] = BORDER_COLOR;
        }
    }
}

void set_pixel(uint16_t *buffer, int x, int y, uint16_t color)
{
    buffer[y * FRAME_WIDTH + x] = color;
}

void draw_block(uint16_t *buffer, int x, int y, uint16_t color)
{
    for (int i = 0; i < BLOCK_SIZE; ++i)
    {
        for (int j = 0; j < BLOCK_SIZE; ++j)
        {
            set_pixel(buffer, x + j, y + i, color);
        }
    }
}

void draw_initial_snake_and_food()
{
    for (int i = 0; i < snake_length; ++i)
    {
        draw_block(framebuffer, snake_x[i] * BLOCK_SIZE, snake_y[i] * BLOCK_SIZE, SNAKE_COLOR);
    }
    draw_block(framebuffer, food_x * BLOCK_SIZE, food_y * BLOCK_SIZE, FOOD_COLOR);
}

void clear_snake_and_food()
{
    // Clear the current snake positions
    for (int i = 0; i < snake_length; ++i)
    {
        draw_block(framebuffer, snake_x[i] * BLOCK_SIZE, snake_y[i] * BLOCK_SIZE, BACKGROUND_COLOR);
    }

    // Clear the current food position
    draw_block(framebuffer, food_x * BLOCK_SIZE, food_y * BLOCK_SIZE, BACKGROUND_COLOR);
}

void reset_game()
{
    clear_snake_and_food();
    snake_length = INITIAL_SNAKE_LENGTH;
    snake_direction = INITIAL_SNAKE_DIRECTION;

    // Set the head of the snake
    snake_x[0] = INITIAL_SNAKE_X;
    snake_y[0] = INITIAL_SNAKE_Y;

    // Initialize the rest of the snake segments
    for (int i = 1; i < snake_length; ++i)
    {
        snake_x[i] = snake_x[i - 1] - 1;
        snake_y[i] = snake_y[i - 1];
    }

    // Initialize food position
    food_x = INITIAL_FOOD_X;
    food_y = INITIAL_FOOD_Y;

    draw_border();
    draw_initial_snake_and_food();  // Draw initial positions without clearing the framebuffer

    // Reset flags
    update_snake = false;
    move_snake_flag = false;
    printf("Game reset\r\n");
}


void move_snake()
{
    int next_x = snake_x[0];
    int next_y = snake_y[0];

    // Determine next position based on the current direction
    switch (snake_direction)
    {
        case 0: next_y -= 1; break; // Up
        case 1: next_x += 1; break; // Right
        case 2: next_y += 1; break; // Down
        case 3: next_x -= 1; break; // Left
    }

    // Collision with the border
    if (next_x <= 0 || next_x >= FRAME_WIDTH / BLOCK_SIZE - 1 || next_y <= 0 || next_y >= FRAME_HEIGHT / BLOCK_SIZE - 1)
    {
        printf("Collision with border\r\n");
        reset_game();
        return;
    }

    // Collision with itself
    for (int i = 1; i < snake_length; ++i)
    {
        if (next_x == snake_x[i] && next_y == snake_y[i])
        {
            printf("Collision with itself\r\n");
            reset_game();
            return;
        }
    }

    // Check if snake eats the food
    if (next_x == food_x && next_y == food_y)
    {
        printf("Food eaten\r\n");
        // Move the snake head to the food position
        for (int i = snake_length; i > 0; --i)
        {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_x[0] = food_x;
        snake_y[0] = food_y;
        snake_length++;
        
        // Generate new food
        do
        {
            food_x = rand() % (FRAME_WIDTH / BLOCK_SIZE);
            food_y = rand() % (FRAME_HEIGHT / BLOCK_SIZE);
        } while (food_x < 1 || food_y < 1 || food_x > (FRAME_WIDTH / BLOCK_SIZE) - 2 || food_y > (FRAME_HEIGHT / BLOCK_SIZE) - 2);
        
        draw_block(framebuffer, food_x * BLOCK_SIZE, food_y * BLOCK_SIZE, FOOD_COLOR);
    }
    else
    {
        // Clear the last segment of the snake if it didn't just eat food
        draw_block(framebuffer, snake_x[snake_length - 1] * BLOCK_SIZE, snake_y[snake_length - 1] * BLOCK_SIZE, BACKGROUND_COLOR);

        // Move the snake forward
        for (int i = snake_length - 1; i > 0; --i)
        {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_x[0] = next_x;
        snake_y[0] = next_y;
    }

    // Redraw each segment of the snake on the display at updated positions
    for (int i = 0; i < snake_length; ++i)
    {
        draw_block(framebuffer, snake_x[i] * BLOCK_SIZE, snake_y[i] * BLOCK_SIZE, SNAKE_COLOR);
    }
}

int main()
{
    board_init();
    tuh_init(BOARD_TUH_RHPORT);
    vreg_set_voltage(VREG_VSEL);
    sleep_ms(10);
    set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

    // Initialize UART for debug output
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart0, true);

    dvi0.timing = &DVI_TIMING;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    multicore_launch_core1(core1_main);

    printf("Game start\r\n");
    initialize_framebuffer();
    reset_game();

    // Set up timer to move the snake
    struct repeating_timer timer;
    add_repeating_timer_ms(SNAKE_MOVE_INTERVAL_MS, repeating_timer_callback, NULL, &timer);

    while (true)
    {
        for (uint y = 0; y < FRAME_HEIGHT; ++y)
        {
            const uint16_t *scanline = &framebuffer[y * FRAME_WIDTH];
            queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
            while (queue_try_remove_u32(&dvi0.q_colour_free, &scanline));
        }
        tuh_task();
        if (move_snake_flag)
        {
            move_snake();  // Move snake when flag is set
            move_snake_flag = false;  // Reset flag after moving
            sleep_ms(1); // Add a small delay to prevent overflow
        }
    }
    return 0;
}
