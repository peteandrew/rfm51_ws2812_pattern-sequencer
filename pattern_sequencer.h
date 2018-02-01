#ifndef PATTERN_SEQUENCER_H__
#define PATTERN_SEQUENCER_H__

#include "neopixel.h"

void pattern_sequencer_init(neopixel_strip_t * strip, uint8_t num_leds);
void pattern_sequencer_command_entry(uint8_t command, uint8_t data[]);
void pattern_sequencer_step();

#endif
