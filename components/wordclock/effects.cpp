#include "wordclock.h"
#include "wordclock_config.h"
#include "color_utils.h"
#include "led_utils.h"
#include "string_pool.h"
#include "light/wordclock_light.h"
#include <cmath>
#include <algorithm>
#include <set>
#include <string>

namespace esphome {
namespace wordclock {

// ============================================================================
// Color Retrieval
// ============================================================================

LightColors WordClock::get_light_colors() {
  LightColors colors;
  colors.hours = get_light_color_safe(hours_light_, HOURS_BRIGHTNESS_RANGE);
  colors.minutes = get_light_color_safe(minutes_light_, MINUTES_BRIGHTNESS_RANGE);
  colors.seconds = get_light_color_safe(seconds_light_, SECONDS_BRIGHTNESS_RANGE);
  colors.background = get_light_color_safe(background_light_, BACKGROUND_BRIGHTNESS_RANGE);
  return colors;
}

EffectParams WordClock::calculate_effect_params() {
  EffectParams params;
  uint32_t now_ms = millis();
  
  params.now_ms = now_ms;
  params.cycle_time = config::calculate_effect_cycle_time(effect_speed_);
  params.pulse_period = config::calculate_effect_period(config::PULSE_PERIOD_BASE_MS, effect_speed_);
  params.breathe_period = config::calculate_effect_period(config::BREATHE_PERIOD_BASE_MS, effect_speed_);
  params.color_cycle_period = config::calculate_effect_period(config::COLOR_CYCLE_PERIOD_BASE_MS, effect_speed_);
  params.words_brightness_mult = words_effect_brightness_ / 100.0f;
  params.seconds_brightness_mult = seconds_effect_brightness_ / 100.0f;
  
  params.hue_time = fmod(1.0f - (float)now_ms / (params.cycle_time * 1000.0f), 1.0f);
  if (params.hue_time < 0) params.hue_time += 1.0f;
  params.hue_per_led = (rainbow_spread_ / 100.0f) * config::HUE_SPREAD_FACTOR;
  
  return params;
}

void WordClock::clear_led_output() {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;
  for (int i = 0; i < num_leds_; i++) {
    (*output)[i] = Color(0, 0, 0);
  }
}

float WordClock::calculate_visual_change_intensity() {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return 0.0f;
  
  float total_change = 0;
  int changed_leds = 0;
  for (int i = 0; i < num_leds_; i++) {
    if (is_excluded_led(i, num_leds_)) continue;
    Color current = Color((*output)[i].get_red(), (*output)[i].get_green(), (*output)[i].get_blue());
    Color previous = prev_led_colors_[i];
    int diff = abs(current.r - previous.r) + abs(current.g - previous.g) + abs(current.b - previous.b);
    if (diff > 0) {
      total_change += diff;
      changed_leds++;
    }
  }
  return changed_leds > 0 ? (total_change / (changed_leds * 765.0f)) : 0.0f;
}

// ============================================================================
// Main Rendering Entry Point
// ============================================================================

void WordClock::apply_light_colors() {
  if (!strip_) return;
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  auto colors = get_light_colors();
  auto params = calculate_effect_params();
  clear_led_output();

  bool words_enabled = (hours_light_ && hours_light_->is_on()) || (minutes_light_ && minutes_light_->is_on());
  if (!words_enabled) {
    typing_in_leds_.clear();
    led_fades_.clear();
  }

  if (words_enabled) {
    apply_words_with_effects(colors, params);
  }
  if (seconds_light_ && seconds_light_->is_on()) {
    apply_seconds_with_effects(colors, params);
  }
  apply_seconds_fades(colors.background, params);
  apply_word_fades(colors.background);
  apply_background(colors.background);

  float change = calculate_visual_change_intensity();
  adaptive_fps_.register_visual_change(change);

  output->schedule_show();
}

// ============================================================================
// Words Rendering with Effects
// ============================================================================

void WordClock::apply_words_with_effects(const LightColors& colors, const EffectParams& params) {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  bool has_effect = (words_effect_ != EFFECT_NONE);
  
  std::vector<int> words_leds;
  if (hours_light_ && hours_light_->is_on()) {
    words_leds.insert(words_leds.end(), active_hours_leds_.begin(), active_hours_leds_.end());
  }
  if (minutes_light_ && minutes_light_->is_on()) {
    words_leds.insert(words_leds.end(), active_minutes_leds_.begin(), active_minutes_leds_.end());
  }

  auto get_fade_in_progress = [&](int led) -> float {
    auto it = typing_in_leds_.find(led);
    if (it == typing_in_leds_.end()) return 1.0f;
    uint32_t start_time = it->second.first;
    int seq = it->second.second;
    float delay = seq * typing_delay_;
    float elapsed = (params.now_ms - start_time) / 1000.0f - delay;
    // FIX H2: Return -1 for LEDs still waiting for their typing delay
    // This signals to skip rendering entirely (not even background)
    if (elapsed < 0) return -1.0f;
    if (words_fade_in_duration_ <= 0) {
      typing_in_leds_.erase(it);
      return 1.0f;
    }
    float progress = elapsed / words_fade_in_duration_;
    if (progress >= 1.0f) {
      typing_in_leds_.erase(it);
      return 1.0f;
    }
    // Ensure minimum visible progress to avoid flash
    return std::max(0.01f, progress);
  };

  if (has_effect) {
    for (size_t i = 0; i < words_leds.size(); i++) {
      int led = words_leds[i];
      if (is_excluded_led(led, num_leds_)) continue;
      float fade_progress = get_fade_in_progress(led);
      // FIX H2: Skip LED entirely if waiting for typing delay (-1)
      // Don't write anything, preserving previous state
      if (fade_progress < 0.0f) {
        continue;
      }
      Color base_color = (std::find(active_hours_leds_.begin(), active_hours_leds_.end(), led) != active_hours_leds_.end()) ? colors.hours : colors.minutes;
      Color color;
      switch (words_effect_) {
        case EFFECT_RAINBOW: {
          float hue = fmod(i * params.hue_per_led + params.hue_time, 1.0f);
          color = hsv_to_rgb(hue, 1.0f, params.words_brightness_mult);
          break;
        }
        case EFFECT_PULSE: {
          float phase = fmod((float)params.now_ms, params.pulse_period) / params.pulse_period;
          float pulse = (sinf(phase * 2.0f * 3.14159f) + 1.0f) / 2.0f;
          pulse = config::PULSE_MIN_INTENSITY + pulse * config::PULSE_INTENSITY_RANGE;
          pulse *= params.words_brightness_mult * 2.0f;
          if (pulse > 1.0f) pulse = 1.0f;
          color = Color(uint8_t(base_color.r * pulse), uint8_t(base_color.g * pulse), uint8_t(base_color.b * pulse));
          break;
        }
        case EFFECT_BREATHE: {
          float phase = fmod((float)params.now_ms, params.breathe_period) / params.breathe_period;
          float breathe = (sinf(phase * 2.0f * 3.14159f) + 1.0f) / 2.0f;
          breathe = config::BREATHE_MIN_INTENSITY + breathe * config::BREATHE_INTENSITY_RANGE;
          breathe *= params.words_brightness_mult * 2.0f;
          if (breathe > 1.0f) breathe = 1.0f;
          color = Color(uint8_t(base_color.r * breathe), uint8_t(base_color.g * breathe), uint8_t(base_color.b * breathe));
          break;
        }
        case EFFECT_COLOR_CYCLE: {
          float t_cycle = fmod((float)params.now_ms, params.color_cycle_period) / params.color_cycle_period;
          color = hsv_to_rgb(t_cycle, 1.0f, params.words_brightness_mult);
          break;
        }
        default:
          color = base_color;
          break;
      }
      if (fade_progress < 1.0f) {
        color = blend_colors(colors.background, color, fade_progress);
      }
      (*output)[led] = color;
      prev_led_colors_[led] = color;
    }
  } else {
    for (int led : active_hours_leds_) {
      if (is_excluded_led(led, num_leds_)) continue;
      float fade_progress = get_fade_in_progress(led);
      // FIX H2: Skip LED entirely if waiting for typing delay
      if (fade_progress < 0.0f) {
        continue;
      }
      Color color = (fade_progress < 1.0f) ? blend_colors(colors.background, colors.hours, fade_progress) : colors.hours;
      (*output)[led] = color;
      prev_led_colors_[led] = color;
    }
    for (int led : active_minutes_leds_) {
      if (is_excluded_led(led, num_leds_)) continue;
      float fade_progress = get_fade_in_progress(led);
      // FIX H2: Skip LED entirely if waiting for typing delay
      if (fade_progress < 0.0f) {
        continue;
      }
      Color color = (fade_progress < 1.0f) ? blend_colors(colors.background, colors.minutes, fade_progress) : colors.minutes;
      (*output)[led] = color;
      prev_led_colors_[led] = color;
    }
  }
}

// ============================================================================
// Seconds Rendering with Effects
// ============================================================================

void WordClock::apply_seconds_with_effects(const LightColors& colors, const EffectParams& params) {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  for (int led : active_seconds_leds_) {
    if (is_excluded_led(led, num_leds_)) continue;
    Color color = colors.seconds;
    switch (seconds_effect_) {
      case EFFECT_RAINBOW:
        color = hsv_to_rgb(params.hue_time, 1.0f, params.seconds_brightness_mult);
        break;
      case EFFECT_PULSE: {
        float phase = fmod((float)params.now_ms, params.pulse_period) / params.pulse_period;
        float pulse = (sinf(phase * 2.0f * 3.14159f) + 1.0f) / 2.0f;
        pulse = config::PULSE_MIN_INTENSITY + pulse * config::PULSE_INTENSITY_RANGE;
        pulse *= params.seconds_brightness_mult * 2.0f;
        if (pulse > 1.0f) pulse = 1.0f;
        color = Color(uint8_t(colors.seconds.r * pulse), uint8_t(colors.seconds.g * pulse), uint8_t(colors.seconds.b * pulse));
        break;
      }
      case EFFECT_BREATHE: {
        float phase = fmod((float)params.now_ms, params.breathe_period) / params.breathe_period;
        float breathe = (sinf(phase * 2.0f * 3.14159f) + 1.0f) / 2.0f;
        breathe = config::BREATHE_MIN_INTENSITY + breathe * config::BREATHE_INTENSITY_RANGE;
        breathe *= params.seconds_brightness_mult * 2.0f;
        if (breathe > 1.0f) breathe = 1.0f;
        color = Color(uint8_t(colors.seconds.r * breathe), uint8_t(colors.seconds.g * breathe), uint8_t(colors.seconds.b * breathe));
        break;
      }
      case EFFECT_COLOR_CYCLE: {
        float t_cycle = fmod((float)params.now_ms, params.color_cycle_period) / params.color_cycle_period;
        color = hsv_to_rgb(t_cycle, 1.0f, params.seconds_brightness_mult);
        break;
      }
      default:
        break;
    }
    (*output)[led] = color;
    prev_led_colors_[led] = color;
  }
}

// ============================================================================
// Fade Effects - Using array-based seconds access
// ============================================================================

void WordClock::apply_seconds_fades(Color background_color, const EffectParams& params) {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  if (!(seconds_light_ && seconds_light_->is_on()) || seconds_fade_out_duration_ <= 0) {
    seconds_fades_.clear();
    return;
  }

  int current_second = last_seconds_;
  int fade_seconds = (int)seconds_fade_out_duration_;
  
  for (int s = 1; s <= fade_seconds && s < 60; s++) {
    int past_second = current_second - s;
    if (past_second <= 0) past_second += 60;
    if (past_second == config::SECONDS_RING_GAP) continue;
    
    const auto& leds = get_second_leds(past_second);
    if (leds.empty()) continue;
    int led = leds[0];
    
    bool is_active = std::find(active_seconds_leds_.begin(), active_seconds_leds_.end(), led) != active_seconds_leds_.end();
    if (is_active) continue;
    
    float progress = (float)s / seconds_fade_out_duration_;
    if (progress < 1.0f) {
      Color from_color = (seconds_effect_ == EFFECT_RAINBOW) 
                         ? hsv_to_rgb(params.hue_time, 1.0f, params.seconds_brightness_mult)
                         : get_light_color_safe(seconds_light_, SECONDS_BRIGHTNESS_RANGE);
      Color blended = blend_colors(from_color, background_color, progress);
      (*output)[led] = blended;
      prev_led_colors_[led] = blended;
    }
  }
}

void WordClock::apply_word_fades(Color background_color) {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;
  uint32_t now_ms = millis();

  for (auto it = led_fades_.begin(); it != led_fades_.end(); ) {
    int led = it->first;
    LedFadeState &fade = it->second;
    
    // Use O(1) led_type_index_ instead of O(n) find
    LightType current_type = get_led_type(led);
    bool is_active = (current_type == LIGHT_HOURS || current_type == LIGHT_MINUTES || current_type == LIGHT_SECONDS);
    if (is_active) {
      it = led_fades_.erase(it);
      continue;
    }
    
    float delay = fade.sequence_index * typing_delay_;
    float elapsed = (now_ms - fade.fade_start) / 1000.0f - delay;
    
    if (elapsed < 0) {
      (*output)[led] = fade.from_color;
      prev_led_colors_[led] = fade.from_color;
      ++it;
    } else {
      float progress = elapsed / fade.fade_duration;
      if (progress >= 1.0f) {
        (*output)[led] = background_color;
        prev_led_colors_[led] = background_color;
        it = led_fades_.erase(it);
      } else {
        Color blended = blend_colors(fade.from_color, background_color, progress);
        (*output)[led] = blended;
        prev_led_colors_[led] = blended;
        ++it;
      }
    }
  }
}

void WordClock::apply_background(Color background_color) {
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;
  if (!(background_light_ && background_light_->is_on())) return;

  for (int led : active_background_leds_) {
    if (is_excluded_led(led, num_leds_)) continue;
    if (led_fades_.find(led) != led_fades_.end()) continue;
    if (seconds_fades_.find(led) != seconds_fades_.end()) continue;
    Color current = Color((*output)[led].get_red(), (*output)[led].get_green(), (*output)[led].get_blue());
    if (current.r > 0 || current.g > 0 || current.b > 0) continue;
    (*output)[led] = background_color;
    prev_led_colors_[led] = background_color;
  }
}

// ============================================================================
// LED Change Detection
// ============================================================================

void WordClock::detect_led_changes() {
  uint32_t now_ms = millis();
  
  std::set<int> current_words_set;
  std::set<int> current_seconds_set;
  for (int led : active_hours_leds_) current_words_set.insert(led);
  for (int led : active_minutes_leds_) current_words_set.insert(led);
  for (int led : active_seconds_leds_) current_seconds_set.insert(led);
  
  if (words_fade_in_duration_ > 0 || typing_delay_ > 0) {
    // Use typing_sequence_ which preserves the order words were added
    // This respects language-specific word order (FR: hours then minutes, EN: minutes then hours)
    std::vector<int> new_words;
    for (int led : typing_sequence_) {
      if (prev_led_types_[led] == LIGHT_BACKGROUND) {
        new_words.push_back(led);
      }
    }
    
    for (size_t seq = 0; seq < new_words.size(); seq++) {
      typing_in_leds_[new_words[seq]] = {now_ms, (int)seq};
    }
  }
  
  std::vector<int> words_fade_out_sequence;
  for (int led : prev_active_words_) {
    if (current_words_set.find(led) == current_words_set.end() &&
        current_seconds_set.find(led) == current_seconds_set.end()) {
      words_fade_out_sequence.push_back(led);
    }
  }
  std::reverse(words_fade_out_sequence.begin(), words_fade_out_sequence.end());
  
  prev_active_words_.clear();
  prev_active_words_.insert(prev_active_words_.end(), active_hours_leds_.begin(), active_hours_leds_.end());
  prev_active_words_.insert(prev_active_words_.end(), active_minutes_leds_.begin(), active_minutes_leds_.end());
  
  std::vector<int> seconds_fade_out;
  for (int i = 0; i < num_leds_; i++) {
    if (is_excluded_led(i, num_leds_)) continue;
    if (prev_led_types_[i] == LIGHT_SECONDS && current_seconds_set.find(i) == current_seconds_set.end()) {
      seconds_fade_out.push_back(i);
    }
  }
  
  if (words_fade_out_duration_ > 0 || typing_delay_ > 0) {
    for (size_t seq = 0; seq < words_fade_out_sequence.size(); seq++) {
      int led = words_fade_out_sequence[seq];
      if (led_fades_.find(led) == led_fades_.end()) {
        LedFadeState fade;
        fade.from_color = prev_led_colors_[led];
        fade.fade_start = now_ms;
        fade.fade_duration = words_fade_out_duration_ > 0 ? words_fade_out_duration_ : 0.01f;
        fade.sequence_index = seq;
        fade.from_type = prev_led_types_[led];
        led_fades_[led] = fade;
      }
    }
  }
  
  if (seconds_fade_out_duration_ > 0) {
    for (int led : seconds_fade_out) {
      LedFadeState fade;
      fade.from_color = prev_led_colors_[led];
      fade.fade_start = now_ms;
      fade.fade_duration = seconds_fade_out_duration_;
      fade.sequence_index = last_seconds_;
      fade.from_type = LIGHT_SECONDS;
      seconds_fades_[led] = fade;
    }
  }
  
  for (int i = 0; i < num_leds_; i++) {
    if (is_excluded_led(i, num_leds_)) continue;
    prev_led_types_[i] = get_led_type(i);
  }
}

// ============================================================================
// Boot Transition
// ============================================================================

void WordClock::apply_boot_transition() {
  if (!strip_ || !power_on_) return;
  auto output = static_cast<light::AddressableLight *>(strip_->get_output());
  if (!output) return;

  uint32_t now_ms = millis();
  float elapsed = (now_ms - boot_transition_start_) / 1000.0f;
  float progress = elapsed / words_fade_out_duration_;
  if (progress > 1.0f) progress = 1.0f;

  for (int i = 0; i < num_leds_; i++) {
    (*output)[i] = Color(0, 0, 0);
  }

  Color background_color = get_light_color_safe(background_light_, BACKGROUND_BRIGHTNESS_RANGE);

  // Get "42" LEDs using StringPool
  size_t key_idx = StringPool::instance().intern("42");
  auto it = ledsarray_misc_.find(key_idx);
  
  float t = fmod(1.0f - (float)now_ms / (config::BOOT_CYCLE_TIME_S * 1000.0f), 1.0f);
  if (t < 0) t += 1.0f;
  float hue_per_led = (config::BOOT_RAINBOW_SPREAD / 100.0f) * config::HUE_SPREAD_FACTOR;

  std::map<int, Color> boot_colors;
  if (it != ledsarray_misc_.end()) {
    const auto& boot_leds = it->second;
    for (size_t i = 0; i < boot_leds.size(); i++) {
      int led = boot_leds[i];
      if (!is_excluded_led(led, num_leds_)) {
        float hue = fmod(i * hue_per_led + t, 1.0f);
        Color color = hsv_to_rgb(hue, 1.0f, config::BOOT_BRIGHTNESS_MULT);
        color = blend_colors(color, Color(0, 0, 0), progress);
        boot_colors[led] = color;
      }
    }
  }

  std::vector<int> time_words_leds;
  time_words_leds.insert(time_words_leds.end(), active_hours_leds_.begin(), active_hours_leds_.end());
  time_words_leds.insert(time_words_leds.end(), active_minutes_leds_.begin(), active_minutes_leds_.end());
  
  float time_hue_per_led = (rainbow_spread_ / 100.0f) * config::HUE_SPREAD_FACTOR;
  float time_words_brightness_mult = words_effect_brightness_ / 100.0f;

  std::map<int, Color> time_colors;
  for (size_t i = 0; i < time_words_leds.size(); i++) {
    int led = time_words_leds[i];
    if (!is_excluded_led(led, num_leds_)) {
      float hue = fmod(i * time_hue_per_led + t, 1.0f);
      Color color = hsv_to_rgb(hue, 1.0f, time_words_brightness_mult);
      color = blend_colors(Color(0, 0, 0), color, progress);
      time_colors[led] = color;
    }
  }

  for (int i = 0; i < num_leds_; i++) {
    if (is_excluded_led(i, num_leds_)) continue;
    Color final_color = background_color;
    if (boot_colors.find(i) != boot_colors.end()) {
      final_color = boot_colors[i];
    }
    if (time_colors.find(i) != time_colors.end()) {
      final_color = blend_colors(final_color, time_colors[i], progress);
    }
    (*output)[i] = final_color;
  }

  // Ring fade out using array access
  Color ring_color = Color(0, 255, 0);
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
      brightness *= (1.0f - progress);
      Color ring_led_color = Color(
        uint8_t(ring_color.r * brightness * config::BOOT_BRIGHTNESS_MULT),
        uint8_t(ring_color.g * brightness * config::BOOT_BRIGHTNESS_MULT),
        uint8_t(ring_color.b * brightness * config::BOOT_BRIGHTNESS_MULT)
      );
      Color existing = Color((*output)[led].get_red(), (*output)[led].get_green(), (*output)[led].get_blue());
      (*output)[led] = Color(
        std::min(255, existing.r + ring_led_color.r),
        std::min(255, existing.g + ring_led_color.g),
        std::min(255, existing.b + ring_led_color.b)
      );
    }
  }

  output->schedule_show();
}

}  // namespace wordclock
}  // namespace esphome
