#pragma once

#include "esphome/core/color.h"
#include <cmath>

namespace esphome {
namespace wordclock {

// Forward declaration
class WordClockLight;

// ============================================================================
// Brightness Mapping Constants
// ============================================================================

static const float HOURS_BRIGHTNESS_MIN = 0.15f;
static const float HOURS_BRIGHTNESS_MAX = 0.75f;
static const float MINUTES_BRIGHTNESS_MIN = 0.15f;
static const float MINUTES_BRIGHTNESS_MAX = 0.75f;
static const float SECONDS_BRIGHTNESS_MIN = 0.15f;
static const float SECONDS_BRIGHTNESS_MAX = 0.75f;
static const float BACKGROUND_BRIGHTNESS_MIN = 0.15f;
static const float BACKGROUND_BRIGHTNESS_MAX = 0.30f;

// ============================================================================
// Structures for Color Management
// ============================================================================

/**
 * @brief Range for brightness mapping
 */
struct LightBrightnessRange {
  float min;
  float max;
};

/**
 * @brief Collection of all light colors for rendering
 */
struct LightColors {
  Color hours;
  Color minutes;
  Color seconds;
  Color background;
};

/**
 * @brief Effect timing parameters
 */
struct EffectParams {
  float cycle_time;           ///< Rainbow cycle time in seconds
  float pulse_period;         ///< Pulse effect period in ms
  float breathe_period;       ///< Breathe effect period in ms
  float color_cycle_period;   ///< Color cycle period in ms
  float words_brightness_mult;    ///< Words brightness multiplier [0-1]
  float seconds_brightness_mult;  ///< Seconds brightness multiplier [0-1]
  float hue_time;             ///< Current hue time offset [0-1]
  float hue_per_led;          ///< Hue increment per LED
  uint32_t now_ms;            ///< Current timestamp
};

// ============================================================================
// Predefined Brightness Ranges
// ============================================================================

static const LightBrightnessRange HOURS_BRIGHTNESS_RANGE = {
  HOURS_BRIGHTNESS_MIN, HOURS_BRIGHTNESS_MAX
};

static const LightBrightnessRange MINUTES_BRIGHTNESS_RANGE = {
  MINUTES_BRIGHTNESS_MIN, MINUTES_BRIGHTNESS_MAX
};

static const LightBrightnessRange SECONDS_BRIGHTNESS_RANGE = {
  SECONDS_BRIGHTNESS_MIN, SECONDS_BRIGHTNESS_MAX
};

static const LightBrightnessRange BACKGROUND_BRIGHTNESS_RANGE = {
  BACKGROUND_BRIGHTNESS_MIN, BACKGROUND_BRIGHTNESS_MAX
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Map a brightness value from [0,1] to [min_val, max_val]
 * @param value Input brightness [0,1]
 * @param min_val Minimum output value
 * @param max_val Maximum output value
 * @return Mapped brightness value
 */
inline float map_brightness(float value, float min_val, float max_val) {
  return min_val + value * (max_val - min_val);
}

// ============================================================================
// HSV Cache
// ============================================================================

/**
 * @brief HSV to RGB cache for fast color conversion
 * 
 * Caches 360 RGB values (1° resolution) for a given saturation and value.
 * Automatically rebuilds cache when s or v change.
 */
class HSVCache {
 public:
  HSVCache() : cached_saturation_(-1.0f), cached_value_(-1.0f) {}

  /**
   * @brief Get RGB color from hue with caching
   * @param h Hue [0,1]
   * @param s Saturation [0,1]
   * @param v Value/Brightness [0,1]
   * @return RGB Color
   */
  Color get_rgb(float h, float s, float v) {
    if (s != cached_saturation_ || v != cached_value_) {
      rebuild_cache(s, v);
    }
    
    h = h - floorf(h);
    int hue_index = static_cast<int>(h * 360.0f) % 360;
    
    return cache_[hue_index];
  }

 private:
  static constexpr int CACHE_SIZE = 360;
  
  Color cache_[CACHE_SIZE];
  float cached_saturation_;
  float cached_value_;

  void rebuild_cache(float s, float v) {
    cached_saturation_ = s;
    cached_value_ = v;
    
    for (int i = 0; i < CACHE_SIZE; i++) {
      float h = static_cast<float>(i) / 360.0f;
      cache_[i] = hsv_to_rgb_uncached(h, s, v);
    }
  }

  static Color hsv_to_rgb_uncached(float h, float s, float v) {
    float r, g, b;
    int i = static_cast<int>(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    
    switch (i % 6) {
      case 0: r = v; g = t; b = p; break;
      case 1: r = q; g = v; b = p; break;
      case 2: r = p; g = v; b = t; break;
      case 3: r = p; g = q; b = v; break;
      case 4: r = t; g = p; b = v; break;
      case 5: r = v; g = p; b = q; break;
      default: r = 0; g = 0; b = 0; break;
    }
    
    return Color(
      static_cast<uint8_t>(r * 255.0f),
      static_cast<uint8_t>(g * 255.0f),
      static_cast<uint8_t>(b * 255.0f)
    );
  }
};

/// Global static HSV cache instance
static HSVCache g_hsv_cache;

// ============================================================================
// Color Function Declarations
// ============================================================================

/**
 * @brief Convert HSV color to RGB using cache
 * @param h Hue [0,1]
 * @param s Saturation [0,1]
 * @param v Value/Brightness [0,1]
 * @return RGB Color
 */
Color hsv_to_rgb(float h, float s, float v);

/**
 * @brief Blend two colors with smooth interpolation (ease in/out)
 * @param from Source color
 * @param to Target color
 * @param progress Blend progress [0,1]
 * @return Blended color
 */
Color blend_colors(Color from, Color to, float progress);

/**
 * @brief Safely get color from a WordClockLight with brightness mapping
 * @param light Pointer to the light (can be nullptr)
 * @param range Brightness range for mapping
 * @return RGB Color (black if light is null or off)
 */
Color get_light_color_safe(WordClockLight* light, const LightBrightnessRange& range);

}  // namespace wordclock
}  // namespace esphome
