#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../wordclock.h"

namespace esphome {
namespace wordclock {

class WordClockSwitch : public switch_::Switch, public Component {
 public:
  void setup() override {
    bool state = true;
    this->pref_ = global_preferences->make_preference<bool>(this->get_object_id_hash());
    this->pref_.load(&state);
    if (wordclock_) wordclock_->set_power_state(state);
    this->publish_state(state);
  }

  void set_wordclock(WordClock *wordclock) { wordclock_ = wordclock; }

 protected:
  void write_state(bool state) override {
    if (wordclock_) wordclock_->set_power_state(state);
    this->pref_.save(&state);
    this->publish_state(state);
  }

  WordClock *wordclock_{nullptr};
  ESPPreferenceObject pref_;
};

}  // namespace wordclock
}  // namespace esphome
