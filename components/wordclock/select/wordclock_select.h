#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "../wordclock.h"

namespace esphome {
namespace wordclock {

class WordClockSecondsSelect : public select::Select, public Component {
 public:
  void setup() override {
    int index = 0;
    this->pref_ = global_preferences->make_preference<int>(this->get_object_id_hash());
    this->pref_.load(&index);
    if (wordclock_) wordclock_->set_seconds_mode(index);
    const auto &options = this->traits.get_options();
    if (index >= 0 && index < (int)options.size()) {
      this->publish_state(options[index]);
    }
  }

  void set_wordclock(WordClock *wordclock) { wordclock_ = wordclock; }

 protected:
  void control(const std::string &value) override {
    const auto &options = this->traits.get_options();
    int index = 0;
    for (size_t i = 0; i < options.size(); i++) {
      if (options[i] == value) { index = i; break; }
    }
    if (wordclock_) wordclock_->set_seconds_mode(index);
    this->pref_.save(&index);
    this->publish_state(value);
  }

  WordClock *wordclock_{nullptr};
  ESPPreferenceObject pref_;
};

class WordClockEffectSelect : public select::Select, public Component {
 public:
  void setup() override {
    int index = 1;  // Default: Rainbow
    this->pref_ = global_preferences->make_preference<int>(this->get_object_id_hash());
    this->pref_.load(&index);
    apply_effect(index);
    const auto &options = this->traits.get_options();
    if (index >= 0 && index < (int)options.size()) {
      this->publish_state(options[index]);
    }
  }

  void set_wordclock(WordClock *wordclock) { wordclock_ = wordclock; }
  void set_light_type(LightType type) { light_type_ = type; }

 protected:
  void control(const std::string &value) override {
    const auto &options = this->traits.get_options();
    int index = 0;
    for (size_t i = 0; i < options.size(); i++) {
      if (options[i] == value) { index = i; break; }
    }
    apply_effect(index);
    this->pref_.save(&index);
    this->publish_state(value);
  }

  void apply_effect(int index) {
    if (!wordclock_) return;
    switch (light_type_) {
      case LIGHT_WORDS: wordclock_->set_words_effect(index); break;
      case LIGHT_SECONDS: wordclock_->set_seconds_effect(index); break;
      default: break;
    }
  }

  WordClock *wordclock_{nullptr};
  LightType light_type_{LIGHT_WORDS};
  ESPPreferenceObject pref_;
};

class WordClockLanguageSelect : public select::Select, public Component {
 public:
  void setup() override {
    int index = 0;  // Default: French (LANG_FRENCH = 0)
    this->pref_ = global_preferences->make_preference<int>(this->get_object_id_hash());
    this->pref_.load(&index);
    if (wordclock_) wordclock_->set_language(index);
    const auto &options = this->traits.get_options();
    if (index >= 0 && index < (int)options.size()) {
      this->publish_state(options[index]);
    }
  }

  void set_wordclock(WordClock *wordclock) { wordclock_ = wordclock; }

 protected:
  void control(const std::string &value) override {
    const auto &options = this->traits.get_options();
    int index = 0;
    for (size_t i = 0; i < options.size(); i++) {
      if (options[i] == value) { index = i; break; }
    }
    if (wordclock_) wordclock_->set_language(index);
    this->pref_.save(&index);
    this->publish_state(value);
  }

  WordClock *wordclock_{nullptr};
  ESPPreferenceObject pref_;
};

}  // namespace wordclock
}  // namespace esphome
