#pragma once

#include <map>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>

namespace esphome {
namespace wordclock {

class WordClock;

/// Type for LED maps using StringPool indices
using IndexedLedMap = std::unordered_map<size_t, std::vector<int>>;

/**
 * @brief Base interface for language implementations
 * 
 * Each language must implement this interface to define:
 * - Word to LED index mappings for the matrix
 * - Time to words conversion logic
 * 
 * To add a new language:
 * 1. Create a derived class (e.g., LanguageSpanish)
 * 2. Implement init_leds_arrays() with your matrix mappings
 * 3. Implement compute_active_leds() with language-specific logic
 * 4. Register in LanguageManager during setup()
 * 
 * @see LanguageFrench, LanguageEnglishUK for implementation examples
 */
class LanguageBase {
 public:
  virtual ~LanguageBase() = default;

  /**
   * @brief Initializes LED mappings for this language
   * 
   * Uses IndexedLedMap with StringPool to save memory.
   * Seconds use a fixed 60-entry array for O(1) access.
   * 
   * @param ledsarray_start Map for start words ("il", "est", "it", "is")
   * @param ledsarray_hours Map for hours ("1"-"12", "midi", "minuit", etc.)
   * @param ledsarray_minutes Map for minutes and modifiers
   * @param seconds_ring_leds Array for seconds ring (index 0-59)
   * @param ledsarray_misc Map for special content ("42" for boot)
   */
  virtual void init_leds_arrays(
    IndexedLedMap& ledsarray_start,
    IndexedLedMap& ledsarray_hours,
    IndexedLedMap& ledsarray_minutes,
    std::array<std::vector<int>, 60>& seconds_ring_leds,
    IndexedLedMap& ledsarray_misc
  ) = 0;

  /**
   * @brief Computes active LEDs for a given time
   * 
   * This method must call clock->add_word_from_map() for each
   * word to display, then clock->compute_seconds_leds() and 
   * clock->compute_background_leds().
   * 
   * @param hours Current hour (0-23)
   * @param minutes Current minutes (0-59)
   * @param seconds Current seconds (0-59)
   * @param clock Pointer to parent WordClock component
   */
  virtual void compute_active_leds(int hours, int minutes, int seconds, WordClock* clock) = 0;

  /**
   * @brief Returns the full language name
   * @return Display name (e.g., "Français", "English UK")
   */
  virtual const char* get_name() const = 0;

  /**
   * @brief Returns the short language code
   * @return ISO code or abbreviation (e.g., "fr", "en_uk")
   */
  virtual const char* get_code() const = 0;

 protected:
  /**
   * @brief Initializes seconds ring (common to all languages)
   * @param seconds_ring Array of 60 entries for seconds LEDs
   */
  static void init_seconds_ring(std::array<std::vector<int>, 60>& seconds_ring) {
    // Clear all entries
    for (auto& v : seconds_ring) v.clear();
    
    // Gaps at 0 and 30 - left empty
    seconds_ring[1] = {8};   seconds_ring[2] = {7};   seconds_ring[3] = {6};
    seconds_ring[4] = {5};   seconds_ring[5] = {4};   seconds_ring[6] = {3};
    seconds_ring[7] = {2};   seconds_ring[8] = {1};   seconds_ring[9] = {30};
    seconds_ring[10] = {33}; seconds_ring[11] = {62}; seconds_ring[12] = {65};
    seconds_ring[13] = {94}; seconds_ring[14] = {97}; seconds_ring[15] = {126};
    seconds_ring[16] = {129}; seconds_ring[17] = {158}; seconds_ring[18] = {161};
    seconds_ring[19] = {190}; seconds_ring[20] = {193}; seconds_ring[21] = {222};
    seconds_ring[22] = {225}; seconds_ring[23] = {254}; seconds_ring[24] = {253};
    seconds_ring[25] = {252}; seconds_ring[26] = {251}; seconds_ring[27] = {250};
    seconds_ring[28] = {249}; seconds_ring[29] = {248};
    // 30 = gap (empty)
    seconds_ring[31] = {247}; seconds_ring[32] = {246}; seconds_ring[33] = {245};
    seconds_ring[34] = {244}; seconds_ring[35] = {243}; seconds_ring[36] = {242};
    seconds_ring[37] = {241}; seconds_ring[38] = {240}; seconds_ring[39] = {239};
    seconds_ring[40] = {208}; seconds_ring[41] = {207}; seconds_ring[42] = {176};
    seconds_ring[43] = {175}; seconds_ring[44] = {144}; seconds_ring[45] = {143};
    seconds_ring[46] = {112}; seconds_ring[47] = {111}; seconds_ring[48] = {80};
    seconds_ring[49] = {79};  seconds_ring[50] = {48};  seconds_ring[51] = {47};
    seconds_ring[52] = {16};  seconds_ring[53] = {15};  seconds_ring[54] = {14};
    seconds_ring[55] = {13};  seconds_ring[56] = {12};  seconds_ring[57] = {11};
    seconds_ring[58] = {10};  seconds_ring[59] = {9};
  }
};

}  // namespace wordclock
}  // namespace esphome
