#pragma once

namespace esphome {
namespace wordclock {

// LEDs to exclude from display (corners)
static const int EXCLUDED_LEDS[] = {0, 31, 32, 63, 64, 95, 96, 127, 128, 159, 160, 191, 192, 223, 224, 255};
static const int EXCLUDED_LEDS_COUNT = 16;

/**
 * Check if a LED index is valid and not in the excluded list
 * This function combines bounds checking with exclusion checking
 * @param led LED index to check
 * @param num_leds Total number of LEDs in the strip (default: 256)
 * @return true if LED should be excluded (out of bounds OR in excluded list)
 */
inline bool is_excluded_led(int led, int num_leds = 256) {
  // Bounds validation first
  if (led < 0 || led >= num_leds) {
    return true;
  }
  
  // Then check exclusion list
  for (int i = 0; i < EXCLUDED_LEDS_COUNT; i++) {
    if (EXCLUDED_LEDS[i] == led) return true;
  }
  return false;
}

/**
 * Get the X position of a LED (accounting for serpentine layout)
 * @param led_index LED index [0-255]
 * @return X position [0-15]
 */
inline int get_led_x(int led_index) {
  int row = led_index / 16;
  int col = led_index % 16;
  return (row % 2 == 0) ? col : (15 - col);
}

}  // namespace wordclock
}  // namespace esphome
