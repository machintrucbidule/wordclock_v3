#pragma once

#include "language_base.h"
#include <map>

namespace esphome {
namespace wordclock {

/**
 * Singleton manager for language implementations
 */
class LanguageManager {
 public:
  static LanguageManager& get_instance() {
    static LanguageManager instance;
    return instance;
  }

  void register_language(int lang_id, LanguageBase* lang) {
    if (languages_.count(lang_id)) {
      delete languages_[lang_id];
    }
    languages_[lang_id] = lang;
  }

  LanguageBase* get_language(int lang_id) {
    if (languages_.count(lang_id)) {
      return languages_[lang_id];
    }
    return nullptr;
  }

  ~LanguageManager() {
    for (auto& pair : languages_) {
      delete pair.second;
    }
  }

 private:
  LanguageManager() = default;
  LanguageManager(const LanguageManager&) = delete;
  LanguageManager& operator=(const LanguageManager&) = delete;

  std::map<int, LanguageBase*> languages_;
};

}  // namespace wordclock
}  // namespace esphome
