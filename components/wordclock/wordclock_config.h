#pragma once

#include <cstdint>

namespace esphome {
namespace wordclock {
namespace config {

// ============================================================================
// Boot Sequence Constants
// ============================================================================

/// Boot timeout - reboot if time not synced after this duration
static constexpr uint32_t BOOT_TIMEOUT_MS = 5 * 60 * 1000;  // 5 minutes

/// Boot display update interval
static constexpr uint32_t BOOT_DISPLAY_UPDATE_MS = 30;

/// Boot ring rotation speed (ms per step)
static constexpr uint32_t BOOT_RING_ROTATION_MS = 120;

/// Boot rainbow parameters
static constexpr float BOOT_RAINBOW_SPREAD = 25.0f;
static constexpr float BOOT_CYCLE_TIME_S = 11.0f;
static constexpr float BOOT_BRIGHTNESS_MULT = 0.5f;

/// Boot ring trail length
static constexpr int BOOT_RING_TRAIL_LENGTH = 135;

// ============================================================================
// Seconds Ring Constants
// ============================================================================

/// Number of LEDs in seconds ring (excluding gaps at 0 and 30)
static constexpr int SECONDS_RING_SIZE = 58;

/// Gap position in seconds ring
static constexpr int SECONDS_RING_GAP = 30;

// ============================================================================
// Effect Timing Constants
// ============================================================================

/// Effect update interval (ms)
static constexpr uint32_t EFFECT_UPDATE_INTERVAL_MS = 20;

/// Effect cycle time calculations
static constexpr float EFFECT_CYCLE_TIME_BASE_S = 45.0f;
static constexpr float EFFECT_CYCLE_TIME_MIN_S = 2.5f;
static constexpr float EFFECT_CYCLE_TIME_MAX_S = 450.0f;

/// Pulse effect timing
static constexpr float PULSE_PERIOD_BASE_MS = 1000.0f;
static constexpr float PULSE_MIN_INTENSITY = 0.3f;
static constexpr float PULSE_INTENSITY_RANGE = 0.7f;

/// Breathe effect timing
static constexpr float BREATHE_PERIOD_BASE_MS = 4000.0f;
static constexpr float BREATHE_MIN_INTENSITY = 0.1f;
static constexpr float BREATHE_INTENSITY_RANGE = 0.9f;

/// Color cycle period
static constexpr float COLOR_CYCLE_PERIOD_BASE_MS = 10000.0f;

/// Effect speed scaling factor
static constexpr float EFFECT_SPEED_SCALE = 50.0f;

// ============================================================================
// Millis Overflow Protection
// ============================================================================

/// Threshold for detecting millis() overflow
static constexpr uint32_t MILLIS_OVERFLOW_THRESHOLD = UINT32_MAX / 2;

// ============================================================================
// Power Calculation Constants (WS2812B ECO)
// ============================================================================

static constexpr float IDLE_CURRENT_MA = 1.0f;
static constexpr float MAX_CURRENT_PER_CHANNEL_MA = 12.0f;
static constexpr float LED_VOLTAGE = 5.0f;

// ============================================================================
// Rainbow Spread Calculation
// ============================================================================

/// Hue spread per LED = (spread / 100) * HUE_SPREAD_FACTOR
static constexpr float HUE_SPREAD_FACTOR = 0.1f;

// ============================================================================
// Helper Functions for Effect Timing
// ============================================================================

/**
 * @brief Calculate effect cycle time based on speed parameter
 * @param speed Effect speed [0-100]
 * @return Cycle time in seconds
 */
inline float calculate_effect_cycle_time(float speed) {
  if (speed <= EFFECT_SPEED_SCALE) {
    return EFFECT_CYCLE_TIME_MAX_S - 
           (speed / EFFECT_SPEED_SCALE) * (EFFECT_CYCLE_TIME_MAX_S - EFFECT_CYCLE_TIME_BASE_S);
  } else {
    return EFFECT_CYCLE_TIME_BASE_S - 
           ((speed - EFFECT_SPEED_SCALE) / EFFECT_SPEED_SCALE) * 
           (EFFECT_CYCLE_TIME_BASE_S - EFFECT_CYCLE_TIME_MIN_S);
  }
}

/**
 * @brief Calculate pulse/breathe period based on speed parameter
 * @param base_period Base period at speed=50 in ms
 * @param speed Effect speed [0-100]
 * @return Period in ms
 */
inline float calculate_effect_period(float base_period, float speed) {
  return base_period * (100.0f - speed + 10.0f) / EFFECT_SPEED_SCALE;
}

}  // namespace config

// ============================================================================
// Default Configuration Values
// ============================================================================

namespace defaults {

/// @name Effect Duration Defaults
/// @{
static constexpr float WORDS_FADE_IN_DURATION = 0.3f;
static constexpr float WORDS_FADE_OUT_DURATION = 1.0f;
static constexpr float SECONDS_FADE_OUT_DURATION = 90.0f;
static constexpr float TYPING_DELAY = 0.13f;
/// @}

/// @name Effect Parameter Defaults
/// @{
static constexpr float RAINBOW_SPREAD = 15.0f;
static constexpr float WORDS_EFFECT_BRIGHTNESS = 50.0f;
static constexpr float SECONDS_EFFECT_BRIGHTNESS = 50.0f;
static constexpr float EFFECT_SPEED = 10.0f;
/// @}

/// @name Effect Type Defaults
/// @{
static constexpr int DEFAULT_WORDS_EFFECT = 1;    // Rainbow
static constexpr int DEFAULT_SECONDS_EFFECT = 1;  // Rainbow
static constexpr int DEFAULT_SECONDS_MODE = 0;    // Current second
/// @}

/// @name Default Colors (RGB float components)
/// @{
// Hours: Teal blue
static constexpr float HOURS_COLOR_R = 0.0f;
static constexpr float HOURS_COLOR_G = 0.5f;
static constexpr float HOURS_COLOR_B = 0.5f;
static constexpr float HOURS_BRIGHTNESS = 0.5f;

// Minutes: Orange
static constexpr float MINUTES_COLOR_R = 1.0f;
static constexpr float MINUTES_COLOR_G = 0.5f;
static constexpr float MINUTES_COLOR_B = 0.0f;
static constexpr float MINUTES_BRIGHTNESS = 0.5f;

// Seconds: Violet
static constexpr float SECONDS_COLOR_R = 0.5f;
static constexpr float SECONDS_COLOR_G = 0.0f;
static constexpr float SECONDS_COLOR_B = 1.0f;
static constexpr float SECONDS_BRIGHTNESS = 0.5f;

// Background: Dark grey (off by default)
static constexpr float BACKGROUND_COLOR_R = 0.1f;
static constexpr float BACKGROUND_COLOR_G = 0.1f;
static constexpr float BACKGROUND_COLOR_B = 0.1f;
static constexpr float BACKGROUND_BRIGHTNESS = 0.1f;
static constexpr bool BACKGROUND_ON = false;
/// @}

}  // namespace defaults

}  // namespace wordclock
}  // namespace esphome
