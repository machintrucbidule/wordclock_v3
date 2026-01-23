#pragma once

#include "language_base.h"
#include "string_pool.h"
#include "wordclock.h"
#include "led_utils.h"
#include "esphome/core/log.h"

namespace esphome {
namespace wordclock {

static const char *const TAG_LANG_FR = "wordclock.lang.fr";

class LanguageFrench : public LanguageBase {
 public:
  void init_leds_arrays(
    IndexedLedMap& ledsarray_start,
    IndexedLedMap& ledsarray_hours,
    IndexedLedMap& ledsarray_minutes,
    std::array<std::vector<int>, 60>& seconds_ring_leds,
    IndexedLedMap& ledsarray_misc
  ) override {
    auto& pool = StringPool::instance();
    
    ledsarray_start.clear();
    ledsarray_hours.clear();
    ledsarray_minutes.clear();
    ledsarray_misc.clear();

    // Start words
    ledsarray_start[pool.intern("il")] = {17, 18};
    ledsarray_start[pool.intern("est")] = {20, 21, 22};

    // Hours
    ledsarray_hours[pool.intern("minuit")] = {24, 25, 26, 27, 28, 29};
    ledsarray_hours[pool.intern("1")] = {46, 45, 44};
    ledsarray_hours[pool.intern("7")] = {43, 42, 41, 40};
    ledsarray_hours[pool.intern("3")] = {40, 39, 38, 37, 36};
    ledsarray_hours[pool.intern("6")] = {36, 35, 34};
    ledsarray_hours[pool.intern("5")] = {49, 50, 51, 52};
    ledsarray_hours[pool.intern("4")] = {52, 53, 54, 55, 56, 57};
    ledsarray_hours[pool.intern("2")] = {58, 59, 60, 61};
    ledsarray_hours[pool.intern("8")] = {78, 77, 76, 75};
    ledsarray_hours[pool.intern("9")] = {74, 73, 72, 71};
    ledsarray_hours[pool.intern("11")] = {69, 68, 67, 66};
    ledsarray_hours[pool.intern("10")] = {81, 82, 83};
    ledsarray_hours[pool.intern("midi")] = {84, 85, 86, 87};
    ledsarray_hours[pool.intern("heure")] = {88, 89, 90, 91, 92};
    ledsarray_hours[pool.intern("s")] = {93};

    // Minutes
    ledsarray_minutes[pool.intern("et")] = {110, 109};
    ledsarray_minutes[pool.intern("moins")] = {108, 107, 106, 105, 104};
    ledsarray_minutes[pool.intern("30")] = {103, 102, 101, 100, 99, 98};
    ledsarray_minutes[pool.intern("le")] = {114, 115};
    ledsarray_minutes[pool.intern("20")] = {116, 117, 118, 119, 120};
    ledsarray_minutes[pool.intern("quart")] = {121, 122, 123, 124, 125};
    ledsarray_minutes[pool.intern("5")] = {142, 141, 140, 139};
    ledsarray_minutes[pool.intern("50")] = {142, 141, 140, 139, 138, 137, 136, 135, 134};
    ledsarray_minutes[pool.intern("11")] = {133, 132, 131, 130};
    ledsarray_minutes[pool.intern("40")] = {145, 146, 147, 148, 149, 150, 151, 152};
    ledsarray_minutes[pool.intern("demie")] = {153, 154, 155, 156, 157};
    ledsarray_minutes[pool.intern("10")] = {174, 173, 172};
    ledsarray_minutes[pool.intern("16")] = {171, 170, 169, 168, 167};
    ledsarray_minutes[pool.intern("12")] = {166, 165, 164, 163, 162};
    ledsarray_minutes[pool.intern("14")] = {177, 178, 179, 180, 181, 182, 183, 184};
    ledsarray_minutes[pool.intern("3")] = {185, 186, 187, 188, 189};
    ledsarray_minutes[pool.intern("4")] = {206, 205, 204, 203, 202, 201};
    ledsarray_minutes[pool.intern("13")] = {203, 202, 201, 200, 199, 198};
    ledsarray_minutes[pool.intern("2")] = {197, 196, 195, 194};
    ledsarray_minutes[pool.intern("et_minutes")] = {209, 210};
    ledsarray_minutes[pool.intern("1")] = {212, 213, 214};
    ledsarray_minutes[pool.intern("7")] = {217, 218, 219, 220};
    ledsarray_minutes[pool.intern("8")] = {237, 236, 235, 234};
    ledsarray_minutes[pool.intern("9")] = {232, 231, 230, 229};
    ledsarray_minutes[pool.intern("6")] = {228, 227, 226};

    // Seconds ring (using base class static method)
    LanguageBase::init_seconds_ring(seconds_ring_leds);

    // Misc
    ledsarray_misc[pool.intern("42")] = {
      145, 146, 147, 148, 149, 150, 151, 152,  // QUARANTE
      197, 196, 195, 194                        // DEUX
    };

    ESP_LOGCONFIG(TAG_LANG_FR, "LED arrays initialized (French), StringPool size: %d", pool.size());
  }

  void compute_active_leds(int hours, int minutes, int seconds, WordClock* clock) override {
    int time_hours = hours;
    int time_minutes = minutes;
    int time_seconds = seconds;
    bool morning = true;

    clock->add_word_from_map("il", LIGHT_HOURS);
    clock->add_word_from_map("est", LIGHT_HOURS);

    if (time_hours == 0) morning = false;
    if (time_hours > 12) {
      time_hours -= 12;
      morning = false;
    }

    bool use_moins = (time_minutes > 30 && time_minutes % 5 == 0);
    if (use_moins) {
      time_hours += 1;
      if (time_hours == 13) time_hours = 1;
    }

    if (time_hours > 0 && time_hours != 12) {
      clock->add_word_from_map(std::to_string(time_hours), LIGHT_HOURS);
      clock->add_word_from_map("heure", LIGHT_HOURS);
      if (time_hours > 1) {
        clock->add_word_from_map("s", LIGHT_HOURS);
      }
    } else if (morning == false) {
      clock->add_word_from_map("minuit", LIGHT_HOURS);
    } else {
      clock->add_word_from_map("midi", LIGHT_HOURS);
    }

    if (use_moins) {
      clock->add_word_from_map("moins", LIGHT_MINUTES);
      if (time_minutes == 45) {
        clock->add_word_from_map("le", LIGHT_MINUTES);
        clock->add_word_from_map("quart", LIGHT_MINUTES);
      } else if (time_minutes == 35) {
        clock->add_word_from_map("20", LIGHT_MINUTES);
        clock->add_word_from_map("5", LIGHT_MINUTES);
      } else if (time_minutes == 40) {
        clock->add_word_from_map("20", LIGHT_MINUTES);
      } else if (time_minutes == 50) {
        clock->add_word_from_map("10", LIGHT_MINUTES);
      } else {
        clock->add_word_from_map("5", LIGHT_MINUTES);
      }
    } else if (time_minutes == 30) {
      clock->add_word_from_map("et", LIGHT_MINUTES);
      clock->add_word_from_map("demie", LIGHT_MINUTES);
    } else if (time_minutes == 15) {
      clock->add_word_from_map("et", LIGHT_MINUTES);
      clock->add_word_from_map("quart", LIGHT_MINUTES);
    } else if (time_minutes > 0) {
      if (time_minutes % 10 == 0 || time_minutes <= 16) {
        clock->add_word_from_map(std::to_string(time_minutes), LIGHT_MINUTES);
      } else {
        int minutes_unit = time_minutes % 10;
        int minutes_tens = time_minutes - minutes_unit;
        
        clock->add_word_from_map(std::to_string(minutes_tens), LIGHT_MINUTES);
        if (minutes_unit == 1) {
          clock->add_word_from_map("et_minutes", LIGHT_MINUTES);
        }
        clock->add_word_from_map(std::to_string(minutes_unit), LIGHT_MINUTES);
      }
    }

    clock->compute_seconds_leds(time_seconds);
    clock->compute_background_leds();
  }

  const char* get_name() const override { return "Français"; }
  const char* get_code() const override { return "fr"; }
};

}  // namespace wordclock
}  // namespace esphome
