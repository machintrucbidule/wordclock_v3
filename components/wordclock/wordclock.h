#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/time/real_time_clock.h"
#include "wordclock_config.h"
#include <array>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

namespace esphome {
namespace wordclock {

// Forward declarations
class LanguageBase;
struct LightColors;
struct EffectParams;
struct LightBrightnessRange;

/// Type for LED maps using StringPool indices
using IndexedLedMap = std::unordered_map<size_t, std::vector<int>>;

// ============================================================================
// Enumerations
// ============================================================================

enum LightType {
  LIGHT_HOURS = 0,
  LIGHT_MINUTES = 1,
  LIGHT_SECONDS = 2,
  LIGHT_BACKGROUND = 3,
  LIGHT_WORDS = 4,
  LIGHT_BOOT = 5
};

enum SecondsMode {
  SECONDS_CURRENT = 0,
  SECONDS_PASSED = 1,
  SECONDS_INVERTED = 2
};

enum EffectType {
  EFFECT_NONE = 0,
  EFFECT_RAINBOW = 1,
  EFFECT_PULSE = 2,
  EFFECT_BREATHE = 3,
  EFFECT_COLOR_CYCLE = 4
};

enum BootState {
  BOOT_WAITING_WIFI = 0,
  BOOT_WAITING_TIME_SYNC = 1,
  BOOT_CAPTIVE_PORTAL = 2,
  BOOT_TRANSITION_TO_TIME = 3,
  BOOT_COMPLETE = 4
};

enum MatrixLanguage {
  LANG_FRENCH = 0,
  LANG_ENGLISH_UK = 1
};

/**
 * @brief Index for Number components used in factory_reset
 */
enum NumberComponentIndex {
  NUM_WORDS_FADE_IN = 0,
  NUM_WORDS_FADE_OUT = 1,
  NUM_SECONDS_FADE_OUT = 2,
  NUM_TYPING_DELAY = 3,
  NUM_RAINBOW_SPREAD = 4,
  NUM_WORDS_EFFECT_BRIGHTNESS = 5,
  NUM_EFFECT_SPEED = 6,
  NUM_SECONDS_EFFECT_BRIGHTNESS = 7
};

// ============================================================================
// Structures
// ============================================================================

struct LedFadeState {
  LightType from_type;
  Color from_color;
  uint32_t fade_start;
  float fade_duration;
  int sequence_index;
};

/**
 * @brief Reusable vector pool to avoid allocations
 */
class LedVectorPool {
 public:
  std::vector<int>& acquire() {
    if (next_free_ >= pool_.size()) {
      pool_.emplace_back();
      pool_.back().reserve(VECTOR_CAPACITY);
    }
    return pool_[next_free_++];
  }
  
  void reset() {
    for (auto& vec : pool_) {
      vec.clear();
    }
    next_free_ = 0;
  }
  
  size_t pool_size() const { return pool_.size(); }
  size_t active_count() const { return next_free_; }
  
 private:
  static constexpr size_t VECTOR_CAPACITY = 256;
  std::vector<std::vector<int>> pool_;
  size_t next_free_{0};
};

/**
 * @brief Adaptive FPS management to save CPU
 */
class AdaptiveFPS {
 public:
  bool should_update(uint32_t now_ms) {
    if (now_ms - last_update_ms_ < current_interval_ms_) {
      return false;
    }
    last_update_ms_ = now_ms;
    
    if (visual_change_rate_ >= CHANGE_THRESHOLD_HIGH) {
      current_interval_ms_ = INTERVAL_HIGH_FPS;
    } else if (visual_change_rate_ >= CHANGE_THRESHOLD_LOW) {
      current_interval_ms_ = INTERVAL_MED_FPS;
    } else {
      current_interval_ms_ = INTERVAL_LOW_FPS;
    }
    
    return true;
  }
  
  void register_visual_change(float intensity) {
    visual_change_rate_ = visual_change_rate_ * 0.9f + intensity * 0.1f;
  }
  
  uint32_t get_current_interval_ms() const { return current_interval_ms_; }
  float get_visual_change_rate() const { return visual_change_rate_; }
  
 private:
  uint32_t last_update_ms_{0};
  uint32_t current_interval_ms_{20};
  float visual_change_rate_{1.0f};
  
  static constexpr uint32_t INTERVAL_HIGH_FPS = 20;
  static constexpr uint32_t INTERVAL_MED_FPS = 50;
  static constexpr uint32_t INTERVAL_LOW_FPS = 100;
  static constexpr float CHANGE_THRESHOLD_HIGH = 0.5f;
  static constexpr float CHANGE_THRESHOLD_LOW = 0.1f;
};

// ============================================================================
// Forward Declarations of Component Classes
// ============================================================================

class WordClockLight;
class WordClockSwitch;
class WordClockSecondsSelect;
class WordClockEffectSelect;
class WordClockLanguageSelect;
class WordClockNumber;

// ============================================================================
// Main WordClock Class
// ============================================================================

class WordClock : public Component {
 public:
  static constexpr size_t NUM_NUMBER_COMPONENTS = 8;
  
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;

  // Configuration
  void set_num_leds(uint16_t num_leds) { num_leds_ = num_leds; }
  void set_time(time::RealTimeClock *time) { time_ = time; }
  void set_strip(light::AddressableLightState *strip) { strip_ = strip; }

  // Component Registration
  void register_light(WordClockLight *light, LightType type);
  void register_switch(WordClockSwitch *sw) { power_switch_ = sw; }
  void register_seconds_select(WordClockSecondsSelect *sel) { seconds_select_ = sel; }
  void register_effect_select(WordClockEffectSelect *sel, LightType type);
  void register_language_select(WordClockLanguageSelect *sel) { language_select_ = sel; }
  void register_number(WordClockNumber *num, int type);
  void register_light_state(light::LightState *state, LightType type);

  // Power Control
  void set_power_state(bool state);
  bool get_power_state() const { return power_on_; }

  // Mode Configuration
  void set_seconds_mode(int mode) { seconds_mode_ = mode; }
  int get_seconds_mode() const { return seconds_mode_; }
  void set_words_effect(int effect) { words_effect_ = effect; }
  int get_words_effect() const { return words_effect_; }
  void set_seconds_effect(int effect) { seconds_effect_ = effect; }
  int get_seconds_effect() const { return seconds_effect_; }

  // Language Management
  void set_language(int lang);
  int get_language() const { return current_language_; }

  // Effect Parameters
  void set_words_fade_in_duration(float seconds) { words_fade_in_duration_ = seconds; }
  float get_words_fade_in_duration() const { return words_fade_in_duration_; }
  void set_words_fade_out_duration(float seconds) { words_fade_out_duration_ = seconds; }
  float get_words_fade_out_duration() const { return words_fade_out_duration_; }
  void set_seconds_fade_out_duration(float seconds) { seconds_fade_out_duration_ = seconds; }
  float get_seconds_fade_out_duration() const { return seconds_fade_out_duration_; }
  void set_typing_delay(float delay) { typing_delay_ = delay; }
  float get_typing_delay() const { return typing_delay_; }
  void set_rainbow_spread(float spread) { rainbow_spread_ = spread; }
  float get_rainbow_spread() const { return rainbow_spread_; }
  void set_words_effect_brightness(float brightness) { words_effect_brightness_ = brightness; }
  float get_words_effect_brightness() const { return words_effect_brightness_; }
  void set_seconds_effect_brightness(float brightness) { seconds_effect_brightness_ = brightness; }
  float get_seconds_effect_brightness() const { return seconds_effect_brightness_; }
  void set_effect_speed(float speed) { effect_speed_ = speed; }
  float get_effect_speed() const { return effect_speed_; }

  // Status & Monitoring
  float get_estimated_power() const { return estimated_power_w_; }

  // Display Control
  void update_display();
  void set_boot_state(BootState state);
  void show_boot_display();
  void factory_reset();

  // Helper Methods for Language Implementations
  void add_word_from_map(const std::string& key, LightType light_type);
  void compute_seconds_leds(int time_seconds);
  void compute_background_leds();
  
  const std::vector<int>& get_second_leds(int second) const {
    static const std::vector<int> empty;
    if (second < 0 || second >= 60) return empty;
    return seconds_ring_leds_[second];
  }

 protected:
  // ==========================================================================
  // Template-based Component Registration
  // ==========================================================================
  
  /**
   * @brief Template for generic component registration by type
   */
  template<typename T>
  void register_component_by_type(T* component, LightType type,
                                   T** hours_ptr, T** minutes_ptr,
                                   T** seconds_ptr, T** background_ptr) {
    if (!component) return;
    
    switch (type) {
      case LIGHT_HOURS:
        *hours_ptr = component;
        break;
      case LIGHT_MINUTES:
        *minutes_ptr = component;
        break;
      case LIGHT_SECONDS:
        *seconds_ptr = component;
        break;
      case LIGHT_BACKGROUND:
        *background_ptr = component;
        break;
      default:
        break;
    }
  }

  // ==========================================================================
  // LED Mapping & Computation
  // ==========================================================================
  
  void init_leds_arrays();
  void compute_active_leds();
  void add_word(const std::vector<int> &leds, LightType light_type);
  void clear_active_leds();
  LightType get_led_type(int led_index);
  void update_led_type_index();

  // Rendering Methods (effects.cpp)
  LightColors get_light_colors();
  EffectParams calculate_effect_params();
  void clear_led_output();
  void apply_words_with_effects(const LightColors& colors, const EffectParams& params);
  void apply_seconds_with_effects(const LightColors& colors, const EffectParams& params);
  void apply_word_fades(Color background_color);
  void apply_seconds_fades(Color background_color, const EffectParams& params);
  void apply_background(Color background_color);
  void apply_light_colors();
  void apply_boot_transition();
  void detect_led_changes();
  void render_boot_matrix(uint32_t now_ms);
  void render_boot_ring(uint32_t now_ms);
  float calculate_visual_change_intensity();

  // Loop Helpers
  void handle_boot_sequence(uint32_t current_millis);
  void handle_time_display(const esphome::ESPTime& now, uint32_t current_millis);

  // Logging
  void log_display_status();

  // ==========================================================================
  // Member Variables
  // ==========================================================================
  
  /// Core Configuration
  uint16_t num_leds_{256};
  time::RealTimeClock *time_{nullptr};
  light::AddressableLightState *strip_{nullptr};

  /// Registered Light Components
  WordClockLight *hours_light_{nullptr};
  WordClockLight *minutes_light_{nullptr};
  WordClockLight *seconds_light_{nullptr};
  WordClockLight *background_light_{nullptr};
  
  /// Registered Light States
  light::LightState *hours_light_state_{nullptr};
  light::LightState *minutes_light_state_{nullptr};
  light::LightState *seconds_light_state_{nullptr};
  light::LightState *background_light_state_{nullptr};
  
  /// Registered Controls
  WordClockSwitch *power_switch_{nullptr};
  WordClockSecondsSelect *seconds_select_{nullptr};
  WordClockEffectSelect *words_effect_select_{nullptr};
  WordClockEffectSelect *seconds_effect_select_{nullptr};
  WordClockLanguageSelect *language_select_{nullptr};

  /// Number Components - Array for simplified factory_reset
  std::array<WordClockNumber*, NUM_NUMBER_COMPONENTS> number_components_{};

  /// State Management
  bool power_on_{true};
  bool time_synced_{false};
  bool updates_enabled_{true};
  uint32_t last_time_check_{0};
  uint32_t setup_time_{0};

  /// Time State
  int last_hours_{-1};
  int last_minutes_{-1};
  int last_seconds_{-1};
  
  /// Boot State
  BootState boot_state_{BOOT_WAITING_WIFI};
  uint32_t boot_transition_start_{0};
  bool first_time_display_{true};

  /// Effect Configuration
  int seconds_mode_{defaults::DEFAULT_SECONDS_MODE};
  int words_effect_{defaults::DEFAULT_WORDS_EFFECT};
  int seconds_effect_{defaults::DEFAULT_SECONDS_EFFECT};
  int current_language_{LANG_FRENCH};
  float words_fade_in_duration_{defaults::WORDS_FADE_IN_DURATION};
  float words_fade_out_duration_{defaults::WORDS_FADE_OUT_DURATION};
  float seconds_fade_out_duration_{defaults::SECONDS_FADE_OUT_DURATION};
  float rainbow_spread_{defaults::RAINBOW_SPREAD};
  float words_effect_brightness_{defaults::WORDS_EFFECT_BRIGHTNESS};
  float seconds_effect_brightness_{defaults::SECONDS_EFFECT_BRIGHTNESS};
  float effect_speed_{defaults::EFFECT_SPEED};
  float typing_delay_{defaults::TYPING_DELAY};
  
  /// Monitoring
  float estimated_power_w_{0.0f};

  /// LED Mappings
  IndexedLedMap ledsarray_start_;
  IndexedLedMap ledsarray_hours_;
  IndexedLedMap ledsarray_minutes_;
  IndexedLedMap ledsarray_misc_;
  std::array<std::vector<int>, 60> seconds_ring_leds_;

  /// LED Vector Pool - Avoids heap allocations
  LedVectorPool led_pool_;
  
  /// Active LED Lists (references from pool)
  std::vector<int> active_hours_leds_;
  std::vector<int> active_minutes_leds_;
  std::vector<int> active_seconds_leds_;
  std::vector<int> active_background_leds_;
  std::vector<int> prev_active_words_;
  
  /// Typing sequence - preserves word addition order for fade-in animation
  std::vector<int> typing_sequence_;

  /// LED Type Index - O(1) lookup
  std::array<LightType, 256> led_type_index_;

  /// Transition State
  std::vector<LightType> prev_led_types_;
  std::vector<Color> prev_led_colors_;
  std::map<int, LedFadeState> led_fades_;
  std::map<int, LedFadeState> seconds_fades_;
  std::map<int, std::pair<uint32_t, int>> typing_in_leds_;
  
  /// Adaptive FPS Controller
  AdaptiveFPS adaptive_fps_;
};

}  // namespace wordclock
}  // namespace esphome
