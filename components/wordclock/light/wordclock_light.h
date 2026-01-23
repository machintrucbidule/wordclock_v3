#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "../wordclock.h"

namespace esphome {
namespace wordclock {

struct LightColorState {
  bool is_on;
  float red;
  float green;
  float blue;
  float brightness;
};

class WordClockLight : public light::LightOutput, public Component {
 public:
  void setup() override {
    // Get default values based on light type
    LightColorState state = get_default_state();
    
    // Create unique hash based on light_type_
    uint32_t hash = fnv1_hash("wordclock_light_" + std::to_string(light_type_));
    
    // Try to load from preferences (will keep default if no saved state)
    this->pref_ = global_preferences->make_preference<LightColorState>(hash);
    this->pref_.load(&state);
    
    // Store for later use
    saved_state_ = state;
    has_restored_ = true;
  }
  
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }

  void write_state(light::LightState *state) override {
    state_ = state;
    
    // Register light state with wordclock on first call
    if (!state_registered_ && wordclock_ && state_) {
      wordclock_->register_light_state(state_, light_type_);
      state_registered_ = true;
    }
    
    // Apply restored state on first write_state call
    if (has_restored_ && !has_applied_) {
      has_applied_ = true;
      auto call = state_->make_call();
      if (saved_state_.is_on) {
        call.set_state(true);
        call.set_red(saved_state_.red);
        call.set_green(saved_state_.green);
        call.set_blue(saved_state_.blue);
        call.set_brightness(saved_state_.brightness);
      } else {
        call.set_state(false);
      }
      call.perform();
    }
    
    // Save current state to preferences
    if (state_) {
      LightColorState current;
      current.is_on = state_->current_values.is_on();
      current.red = state_->current_values.get_red();
      current.green = state_->current_values.get_green();
      current.blue = state_->current_values.get_blue();
      current.brightness = state_->current_values.get_brightness();
      this->pref_.save(&current);
    }
  }

  void set_wordclock(WordClock *wordclock) { wordclock_ = wordclock; }
  void set_light_type(LightType type) { light_type_ = type; }

  void get_rgb(float *r, float *g, float *b, float *brightness) {
    if (state_) {
      *brightness = state_->current_values.get_brightness();
      *r = state_->current_values.get_red();
      *g = state_->current_values.get_green();
      *b = state_->current_values.get_blue();
    } else {
      *r = 1.0f;
      *g = 1.0f;
      *b = 1.0f;
      *brightness = 1.0f;
    }
  }

  bool is_on() {
    if (state_) {
      return state_->current_values.is_on();
    }
    return false;
  }

 protected:
  // FNV-1 hash function
  static uint32_t fnv1_hash(const std::string &str) {
    uint32_t hash = 2166136261UL;
    for (char c : str) {
      hash *= 16777619UL;
      hash ^= c;
    }
    return hash;
  }

  LightColorState get_default_state() {
    LightColorState state;
    state.is_on = true;
    
    switch (light_type_) {
      case LIGHT_HOURS:
        // Bleu pétrole foncé 50%
        state.red = 0.0f;
        state.green = 0.5f;
        state.blue = 0.5f;
        state.brightness = 0.5f;
        break;
      case LIGHT_MINUTES:
        // Orange 50%
        state.red = 1.0f;
        state.green = 0.5f;
        state.blue = 0.0f;
        state.brightness = 0.5f;
        break;
      case LIGHT_SECONDS:
        // Violet 50%
        state.red = 0.5f;
        state.green = 0.0f;
        state.blue = 1.0f;
        state.brightness = 0.5f;
        break;
      case LIGHT_BACKGROUND:
        // Blanc 2%
        state.red = 1.0f;
        state.green = 1.0f;
        state.blue = 1.0f;
        state.brightness = 0.02f;
        break;
      default:
        state.red = 1.0f;
        state.green = 1.0f;
        state.blue = 1.0f;
        state.brightness = 0.5f;
        break;
    }
    return state;
  }

  WordClock *wordclock_{nullptr};
  LightType light_type_{LIGHT_HOURS};
  light::LightState *state_{nullptr};
  ESPPreferenceObject pref_;
  LightColorState saved_state_;
  bool has_restored_{false};
  bool has_applied_{false};
  bool state_registered_{false};
};

}  // namespace wordclock
}  // namespace esphome
