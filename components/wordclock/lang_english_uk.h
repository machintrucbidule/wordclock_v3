#pragma once

#include "language_base.h"
#include "string_pool.h"
#include "wordclock.h"
#include "led_utils.h"
#include "esphome/core/log.h"

namespace esphome {
namespace wordclock {

static const char *const TAG_LANG_UK = "wordclock.lang.en_uk";

class LanguageEnglishUK : public LanguageBase {
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
    ledsarray_start[pool.intern("it")] = {17, 18};
    ledsarray_start[pool.intern("is")] = {20, 21};

    // Hours
    ledsarray_hours[pool.intern("1")] = {196, 195, 194};
    ledsarray_hours[pool.intern("2")] = {198, 197, 196};
    ledsarray_hours[pool.intern("3")] = {174, 173, 172, 171, 170};
    ledsarray_hours[pool.intern("4")] = {169, 168, 167, 166};
    ledsarray_hours[pool.intern("5")] = {165, 164, 163, 162};
    ledsarray_hours[pool.intern("6")] = {178, 179, 180};
    ledsarray_hours[pool.intern("7")] = {182, 183, 184, 185, 186};
    ledsarray_hours[pool.intern("8")] = {202, 201, 200, 199, 198};
    ledsarray_hours[pool.intern("9")] = {205, 204, 203, 202};
    ledsarray_hours[pool.intern("10")] = {217, 218, 219};
    ledsarray_hours[pool.intern("11")] = {238, 237, 236, 235, 234, 233};
    ledsarray_hours[pool.intern("12")] = {114, 115, 116, 117, 118, 119};
    ledsarray_hours[pool.intern("noon")] = {186, 187, 188, 189};
    ledsarray_hours[pool.intern("midnight")] = {210, 211, 212, 213, 214, 215, 216, 217};
    ledsarray_hours[pool.intern("oclock")] = {231, 230, 229, 228, 227, 226};

    // Minutes
    ledsarray_minutes[pool.intern("1")] = {43, 42, 41};
    ledsarray_minutes[pool.intern("2")] = {45, 44, 43};
    ledsarray_minutes[pool.intern("3")] = {39, 38, 37, 36, 35};
    ledsarray_minutes[pool.intern("4")] = {50, 51, 52, 53};
    ledsarray_minutes[pool.intern("5")] = {110, 109, 108, 107};
    ledsarray_minutes[pool.intern("6")] = {54, 55, 56};
    ledsarray_minutes[pool.intern("7")] = {77, 76, 75, 74, 73};
    ledsarray_minutes[pool.intern("8")] = {82, 83, 84, 85, 86};
    ledsarray_minutes[pool.intern("9")] = {73, 72, 71, 70};
    ledsarray_minutes[pool.intern("10")] = {106, 105, 104};
    ledsarray_minutes[pool.intern("11")] = {103, 102, 101, 100, 99, 98};
    ledsarray_minutes[pool.intern("12")] = {114, 115, 116, 117, 118, 119};
    ledsarray_minutes[pool.intern("13")] = {86, 87, 88, 89, 90, 91, 92, 93};
    ledsarray_minutes[pool.intern("14")] = {50, 51, 52, 53, 57, 58, 59, 60};
    ledsarray_minutes[pool.intern("16")] = {54, 55, 56, 57, 58, 59, 60};
    ledsarray_minutes[pool.intern("17")] = {77, 76, 75, 74, 73, 69, 68, 67, 66};
    ledsarray_minutes[pool.intern("18")] = {82, 83, 84, 85, 86, 90, 91, 92, 93};
    ledsarray_minutes[pool.intern("19")] = {73, 72, 71, 70, 69, 68, 67, 66};
    ledsarray_minutes[pool.intern("20")] = {23, 24, 25, 26, 27, 28};
    ledsarray_minutes[pool.intern("half")] = {121, 122, 123, 124};
    ledsarray_minutes[pool.intern("quarter")] = {137, 136, 135, 134, 133, 132, 131};
    ledsarray_minutes[pool.intern("minute")] = {145, 146, 147, 148, 149, 150};
    ledsarray_minutes[pool.intern("minutes")] = {145, 146, 147, 148, 149, 150, 151};
    ledsarray_minutes[pool.intern("past")] = {153, 154, 155, 156};
    ledsarray_minutes[pool.intern("to")] = {156, 157};

    // Seconds ring (using base class static method)
    LanguageBase::init_seconds_ring(seconds_ring_leds);

    // Misc - "FORTY" + "TWO" = "42"
    ledsarray_misc[pool.intern("42")] = {
      165, 164, 163, 162,        // FIVE (row 9)
      169, 168, 167, 166,        // FOUR (row 9)
      174, 173, 172, 171, 170,   // THREE (row 9)
      198, 197, 196              // TWO (row 11)
    };

    ESP_LOGCONFIG(TAG_LANG_UK, "LED arrays initialized (English UK), StringPool size: %d", pool.size());
  }

  void compute_active_leds(int hours, int minutes, int seconds, WordClock* clock) override {
    int time_hours = hours;
    int time_minutes = minutes;
    int time_seconds = seconds;

    clock->add_word_from_map("it", LIGHT_HOURS);
    clock->add_word_from_map("is", LIGHT_HOURS);

    bool use_to = (time_minutes > 30);
    int display_minutes = use_to ? (60 - time_minutes) : time_minutes;
    
    int display_hour = time_hours;
    if (use_to) {
      display_hour = (time_hours + 1) % 24;
    }
    
    bool is_midnight = (display_hour == 0 || display_hour == 24);
    bool is_noon = (display_hour == 12);
    int hour_12 = display_hour;
    if (hour_12 > 12) hour_12 -= 12;
    if (hour_12 == 0) hour_12 = 12;

    if (time_minutes == 0) {
      if (time_hours == 0 || time_hours == 24) {
        clock->add_word_from_map("midnight", LIGHT_HOURS);
      } else if (time_hours == 12) {
        clock->add_word_from_map("noon", LIGHT_HOURS);
      } else {
        int h = time_hours > 12 ? time_hours - 12 : time_hours;
        clock->add_word_from_map(std::to_string(h), LIGHT_HOURS);
        clock->add_word_from_map("oclock", LIGHT_HOURS);
      }
    } else {
      if (display_minutes == 15) {
        clock->add_word_from_map("quarter", LIGHT_MINUTES);
      } else if (display_minutes == 30) {
        clock->add_word_from_map("half", LIGHT_MINUTES);
      } else if (display_minutes % 5 == 0) {
        if (display_minutes == 5) {
          clock->add_word_from_map("5", LIGHT_MINUTES);
        } else if (display_minutes == 10) {
          clock->add_word_from_map("10", LIGHT_MINUTES);
        } else if (display_minutes == 20) {
          clock->add_word_from_map("20", LIGHT_MINUTES);
        } else if (display_minutes == 25) {
          clock->add_word_from_map("20", LIGHT_MINUTES);
          clock->add_word_from_map("5", LIGHT_MINUTES);
        }
      } else {
        if (display_minutes == 1) {
          clock->add_word_from_map("1", LIGHT_MINUTES);
          clock->add_word_from_map("minute", LIGHT_MINUTES);
        } else if (display_minutes >= 2 && display_minutes <= 9) {
          clock->add_word_from_map(std::to_string(display_minutes), LIGHT_MINUTES);
          clock->add_word_from_map("minutes", LIGHT_MINUTES);
        } else if (display_minutes >= 11 && display_minutes <= 19) {
          clock->add_word_from_map(std::to_string(display_minutes), LIGHT_MINUTES);
          clock->add_word_from_map("minutes", LIGHT_MINUTES);
        } else if (display_minutes >= 21 && display_minutes <= 29) {
          clock->add_word_from_map("20", LIGHT_MINUTES);
          int unit = display_minutes % 10;
          clock->add_word_from_map(std::to_string(unit), LIGHT_MINUTES);
          clock->add_word_from_map("minutes", LIGHT_MINUTES);
        }
      }

      if (use_to) {
        clock->add_word_from_map("to", LIGHT_MINUTES);
      } else {
        clock->add_word_from_map("past", LIGHT_MINUTES);
      }

      if (is_midnight) {
        clock->add_word_from_map("midnight", LIGHT_HOURS);
      } else if (is_noon) {
        clock->add_word_from_map("noon", LIGHT_HOURS);
      } else {
        clock->add_word_from_map(std::to_string(hour_12), LIGHT_HOURS);
      }
    }

    clock->compute_seconds_leds(time_seconds);
    clock->compute_background_leds();
  }

  const char* get_name() const override { return "English UK"; }
  const char* get_code() const override { return "en_uk"; }
};

}  // namespace wordclock
}  // namespace esphome
