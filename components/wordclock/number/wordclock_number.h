#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "../wordclock.h"

namespace esphome {
namespace wordclock {

// 0=words_fade_in, 1=words_fade_out, 2=seconds_fade_out, 3=typing_delay, 4=rainbow_spread, 
// 5=words_effect_brightness, 6=effect_speed, 7=seconds_effect_brightness
class WordClockNumber : public number::Number, public Component {
 public:
  void setup() override {
    float value = get_default_value();
    this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
    this->pref_.load(&value);
    this->publish_state(value);
    apply_value(value);
    
    // Register with wordclock for factory reset
    if (wordclock_) {
      wordclock_->register_number(this, number_type_);
    }
  }

  void set_wordclock(WordClock *wordclock) { wordclock_ = wordclock; }
  void set_number_type(int type) { number_type_ = type; }

 protected:
  float get_default_value() {
    switch (number_type_) {
      case 0: return 0.3f;   // words_fade_in (0.3s)
      case 1: return 1.0f;   // words_fade_out (1.0s)
      case 2: return 90.0f;  // seconds_fade_out (90s)
      case 3: return 0.13f;  // typing_delay (0.13s)
      case 4: return 15.0f;  // rainbow_spread (15%)
      case 5: return 50.0f;  // words_effect_brightness (50%)
      case 6: return 10.0f;  // effect_speed (10%)
      case 7: return 50.0f;  // seconds_effect_brightness (50%)
      default: return 0.0f;
    }
  }

  void control(float value) override {
    apply_value(value);
    this->pref_.save(&value);
    this->publish_state(value);
  }
  
  void apply_value(float value) {
    if (!wordclock_) return;
    switch (number_type_) {
      case 0: wordclock_->set_words_fade_in_duration(value); break;
      case 1: wordclock_->set_words_fade_out_duration(value); break;
      case 2: wordclock_->set_seconds_fade_out_duration(value); break;
      case 3: wordclock_->set_typing_delay(value); break;
      case 4: wordclock_->set_rainbow_spread(value); break;
      case 5: wordclock_->set_words_effect_brightness(value); break;
      case 6: wordclock_->set_effect_speed(value); break;
      case 7: wordclock_->set_seconds_effect_brightness(value); break;
    }
  }

  WordClock *wordclock_{nullptr};
  int number_type_{0};
  ESPPreferenceObject pref_;
};

}  // namespace wordclock
}  // namespace esphome
