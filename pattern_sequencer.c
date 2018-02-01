#include "nrf_log.h"

#include "pattern_sequencer.h"


typedef enum {PAUSED, RUNNING, COMMAND_ENTRY} mode_type;

static uint8_t m_num_leds = 0;
static neopixel_strip_t * m_strip;
static mode_type mode = PAUSED;
static uint8_t instructions[2500] = {0x01, 0x03, 0x05, 0x02, 0x01, 0xff, 0x00, 0x00, 0x03, 0x05, 0x00};
static uint16_t instructions_remaining = 2489;
static uint8_t current_instruction = 0;
static uint8_t current_delay = 0;


static void clear_strip()
{
    NRF_LOG_INFO("clear_strip\r\n");

    neopixel_clear(m_strip);
}


static void clear_instructions()
{
    NRF_LOG_INFO("clear_instructions\r\n");

    mode = PAUSED;
    current_instruction = 0;
    current_delay = 0;
    instructions[0] = 0;
    instructions_remaining = 2500;
    clear_strip();
}


static void run()
{
    NRF_LOG_INFO("run\r\n");

    if (mode == RUNNING) return;
    if (!instructions[current_instruction] && current_instruction == 0) return;
    mode = RUNNING;
}


static void pause()
{
    NRF_LOG_INFO("pause\r\n");

    mode = PAUSED;
    current_delay = 0;
}


static void start_instruction_entry()
{
    NRF_LOG_INFO("start_instruction_entry\r\n");

    mode = COMMAND_ENTRY;
    current_instruction = 0;
    current_delay = 0;

    while (instructions[current_instruction]) {
        switch (instructions[current_instruction]) {
            case 0x01:
                current_instruction++;
                break;
            case 0x02:
                current_instruction += 5;
                break;
            case 0x03:
                current_instruction += 2;
                break;
            case 0x04:
                current_instruction += 4;
                break;
        }
    }

    NRF_LOG_INFO("current_instruction: %d\r\n", current_instruction);

    clear_strip();
}


static void exit_instruction_entry()
{
    NRF_LOG_INFO("exit_instruction_entry\r\n");

    instructions[current_instruction] = 0x00;
    mode = PAUSED;
    current_instruction = 0;
}


static void add_instruction(uint8_t instruction, uint8_t data[])
{
    NRF_LOG_INFO("add_instruction\r\n");

    switch (instruction) {
        case 0x01:
            // Add clear display instruction
            if (instructions_remaining < 1) break;
            instructions[current_instruction] = 0x01;
            current_instruction++;
            instructions_remaining--;
            break;
        case 0x02:
            // Add set value instruction
            if (instructions_remaining < 5) break;
            instructions[current_instruction] = 0x02;
            instructions[current_instruction+1] = data[0];
            instructions[current_instruction+2] = data[1];
            instructions[current_instruction+3] = data[2];
            instructions[current_instruction+4] = data[3];
            current_instruction += 5;
            instructions_remaining -= 5;
            break;
        case 0x03:
            // Add delay instruction
            if (instructions_remaining < 2) break;
            instructions[current_instruction] = 0x03;
            instructions[current_instruction+1] = data[0];
            current_instruction += 2;
            instructions_remaining -= 2;
            break;
        case 0x04:
            // Add set all pixels instruction
            if (instructions_remaining < 4) break;
            instructions[current_instruction] = 0x04;
            instructions[current_instruction+1] = data[0];
            instructions[current_instruction+2] = data[1];
            instructions[current_instruction+3] = data[2];
            current_instruction += 4;
            instructions_remaining -= 4;
            break;
    }
}


void pattern_sequencer_init(neopixel_strip_t * strip, uint8_t num_leds)
{
    NRF_LOG_INFO("pattern_sequencer_init\r\n");

    m_strip = strip;
    m_num_leds = num_leds;
    clear_strip();
    run();
}


void pattern_sequencer_command_entry(uint8_t command, uint8_t data[])
{
    NRF_LOG_INFO("pattern_sequencer_command_entry\r\n");

    if (mode != COMMAND_ENTRY) {
        switch (command) {
            case 0x01:
                clear_instructions();
                break;
            case 0x02:
                run();
                break;
            case 0x03:
                pause();
                break;
            case 0x04:
                start_instruction_entry();
                break;
        }
    } else {
        if (command == 0x00) {
            exit_instruction_entry();
        } else {
            add_instruction(command, data);
        }
    }
}


void pattern_sequencer_step()
{
    // If current_delay > 0, decrement by STEP_TIME. If current_delay < STEP_TIME, current_delay = 0
    // Else
    //   Read and execute instructions from current_instruction up delay instruction or 0
    //   If delay instruction increment current_delay by data value
    //   Else set current_instruction = 0

    if (mode != RUNNING) {
        return;
    }

    NRF_LOG_INFO("pattern_sequencer_step\r\n");

    NRF_LOG_INFO("current_delay: %d\r\n", current_delay);
    if (current_delay > 0) {
        current_delay--;
        if (current_delay > 0) return;
    }

    uint8_t led_num, red, green, blue;

    NRF_LOG_INFO("current_instruction: %d\r\n", current_instruction);

    bool delay_instruction = false;
    while (!delay_instruction) {
        switch (instructions[current_instruction]) {
            case 0x01:
                clear_strip();
                current_instruction++;
                break;
            case 0x02:
                led_num = instructions[current_instruction+1];
                red = instructions[current_instruction+2];
                green = instructions[current_instruction+3];
                blue = instructions[current_instruction+4];
                current_instruction += 5;
                neopixel_set_color(m_strip, led_num, red, green, blue);
                NRF_LOG_INFO("set color, %d, %d, %d, %d\r\n", led_num, red, green, blue);
                break;
            case 0x03:
                current_delay = instructions[current_instruction+1];
                current_instruction += 2;
                delay_instruction = true;
                break;
            case 0x04:
                red = instructions[current_instruction+1];
                green = instructions[current_instruction+2];
                blue = instructions[current_instruction+3];
                current_instruction += 4;
                for (uint8_t i=0; i < m_num_leds; i++) {
                    neopixel_set_color(m_strip, i, red, green, blue);
                }
                break;
        }

        if (!instructions[current_instruction]) {
            current_instruction = 0;
        }
    }
}
