#include "wordclock.h"
#include "wordclock_config.h"
#include "color_utils.h"
#include "led_utils.h"
#include "string_pool.h"
#include "light/wordclock_light.h"
#include "number/wordclock_number.h"
#include "select/wordclock_select.h"
#include "switch/wordclock_switch.h"
#include "language_base.h"
#include "language_manager.h"
#include "lang_french.h"
#include "lang_english_uk.h"
#include "esphome/core/log.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/components/light/light_state.h"
#ifdef USE_CAPTIVE_PORTAL
#include "esphome/components/captive_portal/captive_portal.h"
#endif
#include <cmath>
#include <algorithm>
#include <set>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

namespace esphome {
namespace wordclock {

static const char *const TAG = "wordclock";

// ============================================================================
// Color Utilities Implementation
// ============================================================================

Color hsv_to_rgb(float h, float s, float v) {
  return g_hsv_cache.get_rgb(h, s, v);
}

Color blend_colors(Color from, Color to, float progress) {
  progress = progress * progress * (3.0f - 2.0f * progress);
  return Color(
    uint8_t(from.r + (to.r - from.r) * progress),
    uint8_t(from.g + (to.g - from.g) * progress),
    uint8_t(from.b + (to.b - from.b) * progress)
  );
}

Color get_light_color_safe(WordClockLight* light, const LightBrightnessRange& range) {
  if (!light || !light->is_on()) {
    return Color(0, 0, 0);
  }
  float r, g, b, brightness;
  light->get_rgb(&r, &g, &b, &brightness);
  brightness = map_brightness(brightness, range.min, range.max);
  return Color(
    uint8_t(r * brightness * 255),
    uint8_t(g * brightness * 255),
    uint8_t(b * brightness * 255)
  );
}

// ============================================================================
// Component Lifecycle
// ============================================================================

void WordClock::setup() {
  ESP_LOGCONFIG(TAG, "Setting up WordClock...");
  
  StringPool::instance().clear();
  
  LanguageManager::get_instance().register_language(LANG_FRENCH, new LanguageFrench());
  LanguageManager::get_instance().register_language(LANG_ENGLISH_UK, new LanguageEnglishUK());
  
  init_leds_arrays();
  setup_time_ = millis();
  prev_led_types_.resize(num_leds_, LIGHT_BACKGROUND);
  prev_led_colors_.resize(num_leds_, Color(0, 0, 0));
  led_type_index_.fill(LIGHT_BACKGROUND);
  number_components_.fill(nullptr);
  boot_state_ = BOOT_WAITING_WIFI;
  first_time_display_ = true;
  
  ESP_LOGCONFIG(TAG, "WordClock setup complete, StringPool: %d strings", StringPool::instance().size());
}

void WordClock::dump_config() {
  ESP_LOGCONFIG(TAG, "WordClock:");
  ESP_LOGCONFIG(TAG, "  LEDs: %d, Language: %s", num_leds_, 
                current_language_ == LANG_FRENCH ? "French" : "English UK");
  ESP_LOGCONFIG(TAG, "  StringPool: %d, VectorPool: %d", 
                StringPool::instance().size(), led_pool_.pool_size());
}

// ============================================================================
// Main Loop with Adaptive FPS
// ============================================================================

void WordClock::loop() {
  if (!updates_enabled_ || !time_) return;

  auto now = time_->now();
  uint32_t current_millis = millis();

  if (!time_synced_) {
    handle_boot_sequence(current_millis);
    if (!now.is_valid()) return;
    
    time_synced_ = true;
    ESP_LOGI(TAG, "Time synchronized!");
    set_boot_state(BOOT_TRANSITION_TO_TIME);
    
    last_hours_ = now.hour;
    last_minutes_ = now.minute;
    last_seconds_ = now.second;
    compute_active_leds();
    update_led_type_index();
    
    for (int led : active_hours_leds_) {
      if (!is_excluded_led(led, num_leds_)) prev_led_types_[led] = LIGHT_HOURS;
    }
    for (int led : active_minutes_leds_) {
      if (!is_excluded_led(led, num_leds_)) prev_led_types_[led] = LIGHT_MINUTES;
    }
    for (int led : active_seconds_leds_) {
      if (!is_excluded_led(led, num_leds_)) prev_led_types_[led] = LIGHT_SECONDS;
    }
    
    prev_active_words_.clear();
    prev_active_words_.insert(prev_active_words_.end(), active_hours_leds_.begin(), active_hours_leds_.end());
    prev_active_words_.insert(prev_active_words_.end(), active_minutes_leds_.begin(), active_minutes_leds_.end());
    return;
  }

  if (boot_state_ == BOOT_TRANSITION_TO_TIME) {
    apply_boot_transition();
    float elapsed = (current_millis - boot_transition_start_) / 1000.0f;
    if (elapsed >= words_fade_out_duration_) {
      set_boot_state(BOOT_COMPLETE);
      first_time_display_ = false;
    }
    return;
  }

  handle_time_display(now, current_millis);
}

void WordClock::handle_boot_sequence(uint32_t current_millis) {
  uint32_t elapsed_ms = current_millis - setup_time_;
  if (elapsed_ms > config::BOOT_TIMEOUT_MS && elapsed_ms < config::MILLIS_OVERFLOW_THRESHOLD) {
    ESP_LOGE(TAG, "No time sync after 5 minutes, rebooting...");
    esp_restart();
  }
  
  bool wifi_connected = wifi::global_wifi_component->is_connected();
  if (!wifi_connected) {
#ifdef USE_CAPTIVE_PORTAL
    if (captive_portal::global_captive_portal != nullptr && 
        captive_portal::global_captive_portal->is_active()) {
      set_boot_state(BOOT_CAPTIVE_PORTAL);
    } else {
      set_boot_state(BOOT_WAITING_WIFI);
    }
#else
    set_boot_state(BOOT_WAITING_WIFI);
#endif
  } else {
    set_boot_state(BOOT_WAITING_TIME_SYNC);
  }
  
  if (current_millis - last_time_check_ > config::BOOT_DISPLAY_UPDATE_MS) {
    last_time_check_ = current_millis;
    show_boot_display();
  }
}

void WordClock::handle_time_display(const esphome::ESPTime& now, uint32_t current_millis) {
  int current_hours = now.hour;
  int current_minutes = now.minute;
  int current_seconds = now.second;

  bool time_changed = (current_hours != last_hours_ || 
                       current_minutes != last_minutes_ || 
                       current_seconds != last_seconds_);

  if (time_changed) {
    bool second_changed = (current_seconds != last_seconds_);
    last_hours_ = current_hours;
    last_minutes_ = current_minutes;
    last_seconds_ = current_seconds;
    compute_active_leds();
    update_led_type_index();
    detect_led_changes();
    
    // FIX H2+H7: Force immediate render after time change
    // This ensures new LEDs are rendered in the same frame they're added to typing_in_leds_
    // preventing ghost flashes from skipped frames
    update_display();
    adaptive_fps_.register_visual_change(1.0f);
    
    if (second_changed) {
      log_display_status();
    }
    return;
  }

  // Continuous effect/fade updates (separate from time change)
  bool has_effect = (words_effect_ != EFFECT_NONE || seconds_effect_ != EFFECT_NONE);
  bool has_fades = !led_fades_.empty() || !seconds_fades_.empty() || !typing_in_leds_.empty();
  
  if (has_effect || has_fades) {
    float change_intensity = has_effect ? 1.0f : 0.3f;
    if (adaptive_fps_.should_update(current_millis)) {
      adaptive_fps_.register_visual_change(change_intensity);
      update_display();
    }
  }
}

// ============================================================================
// Boot State Management
// ============================================================================

void WordClock::set_boot_state(BootState state) {
  if (boot_state_ != state) {
    ESP_LOGI(TAG, "Boot state: %d -> %d", boot_state_, state);
    boot_state_ = state;
    if (state == BOOT_TRANSITION_TO_TIME) {
      boot_transition_start_ = millis();
    }
  }
}

// ============================================================================
// Boot Display
// ============================================================================

void WordClock::show_boot_display() {
  if (!strip_ || !power_on_) return;
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  for (int i = 0; i < num_leds_; i++) {
    (*output)[i] = Color(0, 0, 0);
  }

  uint32_t now_ms = millis();
  render_boot_matrix(now_ms);
  render_boot_ring(now_ms);
  output->schedule_show();
}

void WordClock::render_boot_matrix(uint32_t now_ms) {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  size_t key_idx = StringPool::instance().intern("42");
  auto it = ledsarray_misc_.find(key_idx);
  if (it == ledsarray_misc_.end() || it->second.empty()) return;
  
  const std::vector<int>& boot_leds = it->second;
  float t = fmod(1.0f - (float)now_ms / (config::BOOT_CYCLE_TIME_S * 1000.0f), 1.0f);
  if (t < 0) t += 1.0f;
  float hue_per_led = (config::BOOT_RAINBOW_SPREAD / 100.0f) * config::HUE_SPREAD_FACTOR;

  for (size_t i = 0; i < boot_leds.size(); i++) {
    int led = boot_leds[i];
    if (!is_excluded_led(led, num_leds_)) {
      float hue = fmod(i * hue_per_led + t, 1.0f);
      (*output)[led] = hsv_to_rgb(hue, 1.0f, config::BOOT_BRIGHTNESS_MULT);
    }
  }
}

void WordClock::render_boot_ring(uint32_t now_ms) {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  Color ring_color;
  switch (boot_state_) {
    case BOOT_WAITING_WIFI:     ring_color = Color(0, 0, 255); break;
    case BOOT_WAITING_TIME_SYNC: ring_color = Color(0, 255, 0); break;
    case BOOT_CAPTIVE_PORTAL:   ring_color = Color(255, 165, 0); break;
    default:                    ring_color = Color(0, 0, 255); break;
  }

  int ring_position = (now_ms / config::BOOT_RING_ROTATION_MS) % config::SECONDS_RING_SIZE;

  for (int i = 1; i <= 59; i++) {
    if (i == config::SECONDS_RING_GAP) continue;
    const auto& leds = get_second_leds(i);
    if (leds.empty()) continue;
    
    int led = leds[0];
    int idx = (i <= 29) ? (i - 1) : (i - 2);
    int distance = idx - ring_position;
    if (distance < 0) distance += config::SECONDS_RING_SIZE;
    
    if (distance < config::BOOT_RING_TRAIL_LENGTH) {
      float brightness = 1.0f - (float)distance / (float)config::BOOT_RING_TRAIL_LENGTH;
      brightness = brightness * brightness;
      (*output)[led] = Color(
        uint8_t(ring_color.r * brightness * config::BOOT_BRIGHTNESS_MULT),
        uint8_t(ring_color.g * brightness * config::BOOT_BRIGHTNESS_MULT),
        uint8_t(ring_color.b * brightness * config::BOOT_BRIGHTNESS_MULT)
      );
    }
  }
}

// ============================================================================
// Language Management
// ============================================================================

void WordClock::set_language(int lang) {
  if (current_language_ != lang) {
    ESP_LOGI(TAG, "Language: %d -> %d", current_language_, lang);
    current_language_ = lang;
    init_leds_arrays();
    
    // FIX H3: Reset stale color data to prevent ghost flashes
    std::fill(prev_led_colors_.begin(), prev_led_colors_.end(), Color(0, 0, 0));
    std::fill(prev_led_types_.begin(), prev_led_types_.end(), LIGHT_BACKGROUND);
    
    if (time_synced_) {
      compute_active_leds();
      update_led_type_index();
      detect_led_changes();
    }
  }
}

void WordClock::init_leds_arrays() {
  auto lang = LanguageManager::get_instance().get_language(current_language_);
  if (lang) {
    lang->init_leds_arrays(ledsarray_start_, ledsarray_hours_, 
                           ledsarray_minutes_, seconds_ring_leds_, 
                           ledsarray_misc_);
  } else {
    ESP_LOGW(TAG, "Language %d not found", current_language_);
  }
}

// ============================================================================
// LED Computation with Vector Pool
// ============================================================================

void WordClock::compute_active_leds() {
  if (last_hours_ < 0 || last_minutes_ < 0 || last_seconds_ < 0) return;
  
  // Clear vectors (keep capacity)
  active_hours_leds_.clear();
  active_minutes_leds_.clear();
  active_seconds_leds_.clear();
  active_background_leds_.clear();
  typing_sequence_.clear();
  
  auto lang = LanguageManager::get_instance().get_language(current_language_);
  if (lang) {
    lang->compute_active_leds(last_hours_, last_minutes_, last_seconds_, this);
  }
}

void WordClock::clear_active_leds() {
  active_hours_leds_.clear();
  active_minutes_leds_.clear();
  active_seconds_leds_.clear();
  active_background_leds_.clear();
}

void WordClock::add_word(const std::vector<int> &leds, LightType light_type) {
  std::vector<int> *target;
  switch (light_type) {
    case LIGHT_HOURS: target = &active_hours_leds_; break;
    case LIGHT_MINUTES: target = &active_minutes_leds_; break;
    case LIGHT_SECONDS: target = &active_seconds_leds_; break;
    case LIGHT_BACKGROUND: target = &active_background_leds_; break;
    default: return;
  }
  target->insert(target->end(), leds.begin(), leds.end());
  
  // Track word order for typing animation (hours and minutes only)
  if (light_type == LIGHT_HOURS || light_type == LIGHT_MINUTES) {
    typing_sequence_.insert(typing_sequence_.end(), leds.begin(), leds.end());
  }
}

// ============================================================================
// LED Type Index - O(1) lookup
// ============================================================================

void WordClock::update_led_type_index() {
  led_type_index_.fill(LIGHT_BACKGROUND);
  for (int led : active_hours_leds_) {
    if (!is_excluded_led(led, num_leds_)) led_type_index_[led] = LIGHT_HOURS;
  }
  for (int led : active_minutes_leds_) {
    if (!is_excluded_led(led, num_leds_)) led_type_index_[led] = LIGHT_MINUTES;
  }
  for (int led : active_seconds_leds_) {
    if (!is_excluded_led(led, num_leds_)) led_type_index_[led] = LIGHT_SECONDS;
  }
}

LightType WordClock::get_led_type(int led_index) {
  if (led_index < 0 || led_index >= num_leds_) return LIGHT_BACKGROUND;
  return led_type_index_[led_index];
}

// ============================================================================
// Component Registration - Using Templates
// ============================================================================

void WordClock::register_light(WordClockLight *light, LightType type) {
  register_component_by_type(light, type,
    &hours_light_, &minutes_light_, &seconds_light_, &background_light_);
}

void WordClock::register_light_state(light::LightState *state, LightType type) {
  register_component_by_type(state, type,
    &hours_light_state_, &minutes_light_state_,
    &seconds_light_state_, &background_light_state_);
}

void WordClock::register_effect_select(WordClockEffectSelect *sel, LightType type) {
  switch (type) {
    case LIGHT_WORDS: words_effect_select_ = sel; break;
    case LIGHT_SECONDS: seconds_effect_select_ = sel; break;
    default: break;
  }
}

void WordClock::register_number(WordClockNumber *num, int type) {
  if (type >= 0 && type < (int)NUM_NUMBER_COMPONENTS) {
    number_components_[type] = num;
  }
}

// ============================================================================
// Factory Reset - Simplified with Arrays and Lambdas
// ============================================================================

void WordClock::factory_reset() {
  ESP_LOGI(TAG, "Factory reset triggered");
  
  updates_enabled_ = false;
  delay(50);

  // Helper lambda for resetting light states
  auto reset_light = [](light::LightState* state, float r, float g, float b, float brightness, bool on = true) {
    if (state) {
      auto call = state->make_call();
      call.set_state(on);
      call.set_rgb(r, g, b);
      call.set_brightness(brightness);
      call.perform();
    }
  };

  // Reset all light states
  reset_light(hours_light_state_, defaults::HOURS_COLOR_R, defaults::HOURS_COLOR_G, 
              defaults::HOURS_COLOR_B, defaults::HOURS_BRIGHTNESS);
  reset_light(minutes_light_state_, defaults::MINUTES_COLOR_R, defaults::MINUTES_COLOR_G,
              defaults::MINUTES_COLOR_B, defaults::MINUTES_BRIGHTNESS);
  reset_light(seconds_light_state_, defaults::SECONDS_COLOR_R, defaults::SECONDS_COLOR_G,
              defaults::SECONDS_COLOR_B, defaults::SECONDS_BRIGHTNESS);
  reset_light(background_light_state_, defaults::BACKGROUND_COLOR_R, defaults::BACKGROUND_COLOR_G,
              defaults::BACKGROUND_COLOR_B, defaults::BACKGROUND_BRIGHTNESS, defaults::BACKGROUND_ON);

  // Reset selects
  if (words_effect_select_) {
    auto call = words_effect_select_->make_call();
    call.set_option("Rainbow");
    call.perform();
  }
  if (seconds_effect_select_) {
    auto call = seconds_effect_select_->make_call();
    call.set_option("Rainbow");
    call.perform();
  }
  if (seconds_select_) {
    auto call = seconds_select_->make_call();
    call.set_option("Current second");
    call.perform();
  }
  set_seconds_mode(defaults::DEFAULT_SECONDS_MODE);

  if (language_select_) {
    auto call = language_select_->make_call();
    call.set_option("Francais");
    call.perform();
  }
  set_language(LANG_FRENCH);

  // Helper lambda for resetting numbers
  auto reset_number = [](WordClockNumber* num, float value) {
    if (num) {
      auto call = num->make_call();
      call.set_value(value);
      call.perform();
    }
  };

  // Reset all number components using array
  reset_number(number_components_[NUM_WORDS_FADE_IN], defaults::WORDS_FADE_IN_DURATION);
  reset_number(number_components_[NUM_WORDS_FADE_OUT], defaults::WORDS_FADE_OUT_DURATION);
  reset_number(number_components_[NUM_SECONDS_FADE_OUT], defaults::SECONDS_FADE_OUT_DURATION);
  reset_number(number_components_[NUM_TYPING_DELAY], defaults::TYPING_DELAY);
  reset_number(number_components_[NUM_RAINBOW_SPREAD], defaults::RAINBOW_SPREAD);
  reset_number(number_components_[NUM_WORDS_EFFECT_BRIGHTNESS], defaults::WORDS_EFFECT_BRIGHTNESS);
  reset_number(number_components_[NUM_SECONDS_EFFECT_BRIGHTNESS], defaults::SECONDS_EFFECT_BRIGHTNESS);
  reset_number(number_components_[NUM_EFFECT_SPEED], defaults::EFFECT_SPEED);

  set_power_state(true);
  if (power_switch_) {
    power_switch_->publish_state(true);
  }

  updates_enabled_ = true;
  ESP_LOGI(TAG, "Factory reset complete");
}

// ============================================================================
// Power & Display Control
// ============================================================================

void WordClock::set_power_state(bool state) {
  if (power_on_ != state) {
    power_on_ = state;
    ESP_LOGI(TAG, "Power: %s", state ? "ON" : "OFF");
    update_display();
  }
}

void WordClock::update_display() {
  if (!strip_) return;

  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  if (!power_on_) {
    for (int i = 0; i < num_leds_; i++) {
      (*output)[i] = Color(0, 0, 0);
    }
    output->schedule_show();
    return;
  }

  if (time_synced_ && boot_state_ == BOOT_COMPLETE) {
    apply_light_colors();
  }
}

// ============================================================================
// Status Logging
// ============================================================================

void WordClock::log_display_status() {
  if (!strip_) return;
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  float total_power_mw = 0;
  for (int i = 0; i < num_leds_; i++) {
    if (is_excluded_led(i, num_leds_)) continue;
    uint8_t r = (*output)[i].get_red();
    uint8_t g = (*output)[i].get_green();
    uint8_t b = (*output)[i].get_blue();
    float led_current_ma = config::IDLE_CURRENT_MA;
    led_current_ma += (r / 255.0f) * config::MAX_CURRENT_PER_CHANNEL_MA;
    led_current_ma += (g / 255.0f) * config::MAX_CURRENT_PER_CHANNEL_MA;
    led_current_ma += (b / 255.0f) * config::MAX_CURRENT_PER_CHANNEL_MA;
    total_power_mw += led_current_ma * config::LED_VOLTAGE;
  }
  
  estimated_power_w_ = total_power_mw / 1000.0f;
  int words_count = active_hours_leds_.size() + active_minutes_leds_.size();
  
  float ram_usage = 0;
#ifdef USE_ESP32
  uint32_t free_heap = esp_get_free_heap_size();
  uint32_t total_heap = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  if (total_heap > 0) {
    ram_usage = 100.0f * (1.0f - (float)free_heap / (float)total_heap);
  }
#endif
  
  const char* lang_str = (current_language_ == LANG_FRENCH) ? "FR" : "UK";
  
  ESP_LOGD(TAG, "%02d:%02d:%02d [%s] W:%d S:%d | %.2fW | RAM:%.1f%% | %dms",
    last_hours_, last_minutes_, last_seconds_, lang_str,
    words_count, (int)active_seconds_leds_.size(),
    estimated_power_w_, ram_usage, adaptive_fps_.get_current_interval_ms()
  );
}

// ============================================================================
// Helper Methods for Language Implementations
// ============================================================================

void WordClock::add_word_from_map(const std::string& key, LightType light_type) {
  size_t key_idx = StringPool::instance().intern(key);
  
  IndexedLedMap* source = nullptr;
  switch (light_type) {
    case LIGHT_HOURS:
      if (ledsarray_start_.count(key_idx)) source = &ledsarray_start_;
      else if (ledsarray_hours_.count(key_idx)) source = &ledsarray_hours_;
      break;
    case LIGHT_MINUTES:
      if (ledsarray_minutes_.count(key_idx)) source = &ledsarray_minutes_;
      break;
    default:
      break;
  }
  
  if (source && source->count(key_idx)) {
    add_word((*source)[key_idx], light_type);
  }
}

void WordClock::compute_seconds_leds(int time_seconds) {
  bool seconds_enabled = (seconds_light_ && seconds_light_->is_on());
  
  if (!seconds_enabled || time_seconds == 0 || time_seconds == config::SECONDS_RING_GAP) {
    if (seconds_enabled && (time_seconds == 0 || time_seconds == config::SECONDS_RING_GAP)) {
      if (seconds_mode_ == SECONDS_PASSED && time_seconds == config::SECONDS_RING_GAP) {
        for (int s = 1; s < config::SECONDS_RING_GAP; s++) {
          const auto& leds = get_second_leds(s);
          if (!leds.empty()) add_word(leds, LIGHT_SECONDS);
        }
      } else if (seconds_mode_ == SECONDS_INVERTED) {
        for (int s = 1; s <= 59; s++) {
          if (s == config::SECONDS_RING_GAP) continue;
          const auto& leds = get_second_leds(s);
          if (!leds.empty()) add_word(leds, LIGHT_SECONDS);
        }
      }
    }
    return;
  }

  switch (seconds_mode_) {
    case SECONDS_CURRENT: {
      const auto& leds = get_second_leds(time_seconds);
      if (!leds.empty()) add_word(leds, LIGHT_SECONDS);
      break;
    }
    case SECONDS_PASSED:
      for (int s = 1; s <= time_seconds; s++) {
        if (s == config::SECONDS_RING_GAP) continue;
        const auto& leds = get_second_leds(s);
        if (!leds.empty()) add_word(leds, LIGHT_SECONDS);
      }
      break;
    case SECONDS_INVERTED:
      for (int s = 1; s <= 59; s++) {
        if (s == config::SECONDS_RING_GAP || s == time_seconds) continue;
        const auto& leds = get_second_leds(s);
        if (!leds.empty()) add_word(leds, LIGHT_SECONDS);
      }
      break;
  }
}

void WordClock::compute_background_leds() {
  std::vector<bool> used_leds(num_leds_, false);
  
  for (int led : active_hours_leds_) 
    if (!is_excluded_led(led, num_leds_)) used_leds[led] = true;
  for (int led : active_minutes_leds_) 
    if (!is_excluded_led(led, num_leds_)) used_leds[led] = true;
  for (int led : active_seconds_leds_) 
    if (!is_excluded_led(led, num_leds_)) used_leds[led] = true;

  for (int i = 0; i < num_leds_; i++) {
    if (!used_leds[i] && !is_excluded_led(i, num_leds_)) {
      active_background_leds_.push_back(i);
    }
  }
}

}  // namespace wordclock
}  // namespace esphome
