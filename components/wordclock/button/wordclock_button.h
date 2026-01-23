#pragma once

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "../wordclock.h"

namespace esphome {
namespace wordclock {

class WordClockFactoryResetButton : public button::Button, public Component {
 public:
  void set_wordclock(WordClock *wordclock) { wordclock_ = wordclock; }

 protected:
  void press_action() override {
    if (wordclock_) {
      wordclock_->factory_reset();
    }
  }

  WordClock *wordclock_{nullptr};
};

}  // namespace wordclock
}  // namespace esphome
