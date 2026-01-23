# WordClock Developer Guide

This comprehensive guide provides all the technical information needed to understand, modify, and extend the WordClock project.

## About This Document

**Purpose**: This is an onboarding guide for developers. It describes how the application works **today**, not its development history.

**Maintenance Rules**:
- This guide must always reflect the **current** state of the code
- When code changes, update this guide to match the new behavior
- **Do not** describe historical changes or version differences
- **Do not** use "before/after" comparisons or migration notes
- Remove outdated sections; add new ones as needed
- Keep examples up-to-date with actual code

## Code Conventions

**Language**: All code comments and variable names **must be in English**.

```cpp
// CORRECT
int active_hours_leds_;      // Hours display LEDs
float brightness_mult_;       // Brightness multiplier

// INCORRECT  
int leds_heures_actifs_;     // LEDs des heures
float mult_luminosite_;      // Multiplicateur de luminosité
```

**Naming**:
- Variables: `snake_case` (e.g., `led_type_index_`)
- Classes: `PascalCase` (e.g., `WordClock`, `LedVectorPool`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `NUM_LEDS`, `BOOT_TIMEOUT_MS`)
- Member variables: trailing underscore (e.g., `power_on_`)

---

## Table of Contents
1. [Code Organization](#1-code-organization)
2. [Default Configuration Values](#2-default-configuration-values)
3. [LED Matrix Mapping - French](#3-led-matrix-mapping---french)
4. [LED Matrix Mapping - English UK](#4-led-matrix-mapping---english-uk)
5. [Physical LED Wiring](#5-physical-led-wiring)
6. [Language Implementation Guide](#6-language-implementation-guide)
7. [Effect System](#7-effect-system)
8. [Boot Sequence](#8-boot-sequence)
9. [Architecture Overview](#9-architecture-overview)
10. [Performance Optimizations](#10-performance-optimizations)
11. [Advanced Topics](#11-advanced-topics)

---

## 1. Code Organization

### Main Components

| File | Purpose | Lines (approx) |
|------|---------|----------------|
| `wordclock.cpp/h` | Core component, state management | ~450 |
| `effects.cpp` | Visual effects and rendering | ~400 |
| `wordclock_config.h` | All configuration constants | ~180 |
| `color_utils.h` | Color conversion, HSV cache, structures | ~180 |
| `led_utils.h` | LED indexing utilities | ~30 |
| `language_base.h` | Language interface | ~60 |
| `lang_*.h` | Language implementations | ~200 each |


### Effect Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│                   apply_light_colors()                   │
├─────────────────────────────────────────────────────────┤
│ 1. get_light_colors()                                   │
│    └─> Retrieve base colors from all 4 lights           │
│                                                         │
│ 2. calculate_effect_params()                            │
│    └─> Compute timing (cycle_time, periods, hue_time)   │
│                                                         │
│ 3. clear_led_output()                                   │
│    └─> Set all LEDs to black                            │
│                                                         │
│ 4. apply_words_with_effects(colors, params)             │
│    └─> Render words with rainbow/pulse/breathe/cycle    │
│                                                         │
│ 5. apply_seconds_with_effects(colors, params)           │
│    └─> Render seconds ring with effects                 │
│                                                         │
│ 6. apply_seconds_fades(background, params)              │
│    └─> Blend fading seconds toward background           │
│                                                         │
│ 7. apply_word_fades(background)                         │
│    └─> Blend fading words toward background             │
│                                                         │
│ 8. apply_background(background)                         │
│    └─> Fill remaining LEDs with background color        │
│                                                         │
│ 9. output->schedule_show()                              │
│    └─> Push to LED strip                                │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Default Configuration Values

All defaults are centralized in `wordclock_config.h::defaults`:

### Effect Duration Defaults

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `WORDS_FADE_IN_DURATION` | 0.3s | 0-10s | Word fade-in duration |
| `WORDS_FADE_OUT_DURATION` | 1.0s | 0-10s | Word fade-out duration |
| `SECONDS_FADE_OUT_DURATION` | 90s | 0-180s | Seconds trail duration |
| `TYPING_DELAY` | 0.13s | 0-0.5s | Delay between letters |

### Effect Parameter Defaults

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `RAINBOW_SPREAD` | 15% | 0-100% | Rainbow color spread |
| `WORDS_EFFECT_BRIGHTNESS` | 50% | 0-100% | Words effect brightness |
| `SECONDS_EFFECT_BRIGHTNESS` | 50% | 0-100% | Seconds effect brightness |
| `EFFECT_SPEED` | 10% | 0-100% | Animation speed |

### Mode Defaults

| Parameter | Default | Description |
|-----------|---------|-------------|
| `DEFAULT_WORDS_EFFECT` | 1 (Rainbow) | Initial words effect |
| `DEFAULT_SECONDS_EFFECT` | 1 (Rainbow) | Initial seconds effect |
| `DEFAULT_SECONDS_MODE` | 0 (Current) | Seconds display mode |
| Language | French | Default matrix language |

### Default Colors

| Light | Color | RGB | Brightness |
|-------|-------|-----|------------|
| Hours | Teal blue | (0, 0.5, 0.5) | 50% |
| Minutes | Orange | (1, 0.5, 0) | 50% |
| Seconds | Violet | (0.5, 0, 1) | 50% |
| Background | Dark grey | (0.1, 0.1, 0.1) | 10% (off) |

---

## 3. LED Matrix Mapping - French

The French matrix layout (13×14 characters):

```
ILTESTDMINUIT
UNESEPTROISIX
CINQUATREDEUX
HUITNEUFJONZE
DIXMIDIHEURES
ETMOINSTRENTE
FLEVINGTQUART
CINQUANTEONZE
QUARANTEDEMIE
DIXSEIZEDOUZE
QUATORZETROIS
QUATREIZEDEUX
ETFUNEMGSEPTL
THUITPNEUFSIX
```

### French Word Mappings

**Start words:**
- IL = row 0, col 0-1
- EST = row 0, col 3-5

**Hours:**
- MINUIT = row 0, col 7-12
- UNE = row 1, col 0-2
- SEPT (hour) = row 1, col 3-6
- TROIS (hour) = row 1, col 6-10
- SIX (hour) = row 1, col 10-12
- CINQ (hour) = row 2, col 0-3
- QUATRE (hour) = row 2, col 3-8
- DEUX (hour) = row 2, col 9-12
- HUIT = row 3, col 0-3
- NEUF = row 3, col 4-7
- ONZE = row 3, col 9-12
- DIX (hour) = row 4, col 0-2
- MIDI = row 4, col 3-6
- HEURE(S) = row 4, col 7-12

**Minutes:**
- ET = row 5, col 0-1
- MOINS = row 5, col 2-6
- TRENTE = row 5, col 7-12
- LE = row 6, col 1-2
- VINGT = row 6, col 3-7
- QUART = row 6, col 8-12
- CINQ (min) = row 7, col 0-3
- CINQUANTE = row 7, col 0-8
- ONZE (min) = row 7, col 9-12
- QUARANTE = row 8, col 0-7
- DEMIE = row 8, col 8-12
- DIX (min) = row 9, col 0-2
- SEIZE = row 9, col 3-7
- DOUZE = row 9, col 8-12
- QUATORZE = row 10, col 0-7
- TROIS (min) = row 10, col 8-12
- QUATRE (min) = row 11, col 0-5
- TREIZE = row 11, col 3-8
- DEUX (min) = row 11, col 9-12
- ET (min) = row 12, col 0-1
- UNE (min) = row 12, col 3-5
- SEPT (min) = row 12, col 8-11
- HUIT (min) = row 13, col 1-4
- NEUF (min) = row 13, col 6-9
- SIX (min) = row 13, col 10-12

---

## 4. LED Matrix Mapping - English UK

The English UK matrix layout (13×14 characters):

```
ITRISYTWENTYK
VTWONEBTHREEC
AFOURSIXTEENE
NSEVENINETEEN
CEIGHTHIRTEEN
FIVETENELEVEN
HTWELVEIHALFJ
LIFEKQUARTERM
MINUTESWPASTO
THREEFOURFIVE
NSIXOSEVENOON
PNINEIGHTWONE
QMIDNIGHTENET
ELEVENLOCLOCK
```

### English UK Word Mappings

**Start words:**
- IT = row 0, col 0-1
- IS = row 0, col 3-4

**Minutes (units):**
- ONE = row 1, col 3-5
- TWO = row 1, col 1-3
- THREE = row 1, col 7-11
- FOUR = row 2, col 1-4
- FIVE = row 5, col 0-3
- SIX = row 2, col 5-7
- SEVEN = row 3, col 1-5
- EIGHT = row 4, col 1-5
- NINE = row 3, col 5-8
- TEN = row 5, col 4-6
- ELEVEN = row 5, col 7-12
- TWELVE = row 6, col 1-6

**Minutes (modifiers):**
- TWENTY = row 0, col 6-11
- HALF = row 6, col 8-11
- QUARTER = row 7, col 5-11
- MINUTE = row 8, col 0-5
- MINUTES = row 8, col 0-6
- PAST = row 8, col 8-11
- TO = row 8, col 11-12

**Hours:**
- NOON = row 10, col 9-12
- MIDNIGHT = row 12, col 1-8
- OCLOCK = row 13, col 7-12

---

## 5. Physical LED Wiring

### Physical Structure

- **SECONDS RING**: 58 LEDs surrounding the matrix
- **MATRIX**: 13 columns × 14 rows = 182 LEDs
- **TOTAL**: 256 LEDs (indices 0-255)

### LED Numbering Pattern (Serpentine)

```
Row  0: LEDs  17- 29 (13 LEDs) L→R   [col 0 = LED 17,  col 12 = LED 29]
Row  1: LEDs  34- 46 (13 LEDs) R→L   [col 0 = LED 46,  col 12 = LED 34]
Row  2: LEDs  49- 61 (13 LEDs) L→R   [col 0 = LED 49,  col 12 = LED 61]
Row  3: LEDs  66- 78 (13 LEDs) R→L   [col 0 = LED 78,  col 12 = LED 66]
Row  4: LEDs  81- 93 (13 LEDs) L→R   [col 0 = LED 81,  col 12 = LED 93]
Row  5: LEDs  98-110 (13 LEDs) R→L   [col 0 = LED 110, col 12 = LED 98]
Row  6: LEDs 113-125 (13 LEDs) L→R   [col 0 = LED 113, col 12 = LED 125]
Row  7: LEDs 130-142 (13 LEDs) R→L   [col 0 = LED 142, col 12 = LED 130]
Row  8: LEDs 145-157 (13 LEDs) L→R   [col 0 = LED 145, col 12 = LED 157]
Row  9: LEDs 162-174 (13 LEDs) R→L   [col 0 = LED 174, col 12 = LED 162]
Row 10: LEDs 177-189 (13 LEDs) L→R   [col 0 = LED 177, col 12 = LED 189]
Row 11: LEDs 194-206 (13 LEDs) R→L   [col 0 = LED 206, col 12 = LED 194]
Row 12: LEDs 209-221 (13 LEDs) L→R   [col 0 = LED 209, col 12 = LED 221]
Row 13: LEDs 226-238 (13 LEDs) R→L   [col 0 = LED 238, col 12 = LED 226]
```

### LED Position Formula

```python
def get_led_pos(row, col):
    row_ranges = [
        (17, 29), (34, 46), (49, 61), (66, 78),
        (81, 93), (98, 110), (113, 125), (130, 142),
        (145, 157), (162, 174), (177, 189), (194, 206),
        (209, 221), (226, 238)
    ]
    start, end = row_ranges[row]
    if row % 2 == 0:  # Even row: L→R
        return start + col
    else:  # Odd row: R→L
        return end - col
```

### Seconds Ring Mapping

| Second | LED | Second | LED |
|--------|-----|--------|-----|
| 1 | 8 | 31 | 247 |
| 2 | 7 | 32 | 246 |
| ... | ... | ... | ... |
| 29 | 248 | 59 | 9 |
| 30 | (gap) | 0 | (gap) |

---

## 6. Language Implementation Guide

### Creating a New Language

1. **Create header file** `lang_<language>.h`
2. **Inherit from LanguageBase**
3. **Implement init_leds_arrays()** with word→LED mappings
4. **Implement compute_active_leds()** with time→words logic
5. **Register in setup()** via LanguageManager

### Example Template

```cpp
#pragma once
#include "language_base.h"
#include "wordclock.h"

namespace esphome {
namespace wordclock {

class LanguageSpanish : public LanguageBase {
 public:
  void init_leds_arrays(/* ... */) override {
    // Map Spanish words to LED indices
    ledsarray_start["es"] = {17, 18};
    ledsarray_start["la"] = {20, 21};
    // ... more mappings
  }

  void compute_active_leds(int hours, int minutes, int seconds, 
                           WordClock* clock) override {
    clock->add_word_from_map("es", LIGHT_HOURS);
    clock->add_word_from_map("la", LIGHT_HOURS);
    // ... time logic
    clock->compute_seconds_leds(seconds);
    clock->compute_background_leds();
  }

  const char* get_name() const override { return "Español"; }
  const char* get_code() const override { return "es"; }
};

}  // namespace wordclock
}  // namespace esphome
```

### Typing Animation Order

The fade-in animation displays words in the exact order they are added via `add_word_from_map()` in `compute_active_leds()`. This means:

- **French**: Words added as "IL EST TROIS HEURES VINGT" → appear in that order
- **English**: Words added as "IT IS TWENTY PAST THREE" → appear in that order

The system automatically preserves the language-specific word order. No special configuration is needed.

---

## 7. Effect System

### Available Effects

| ID | Name | Description |
|----|------|-------------|
| 0 | None | Static color |
| 1 | Rainbow | Animated hue shift per LED |
| 2 | Pulse | Fast brightness pulsation |
| 3 | Breathe | Slow brightness breathing |
| 4 | Color Cycle | All LEDs cycle hue together |

### Effect Timing (from config)

| Parameter | Formula |
|-----------|---------|
| cycle_time | `calculate_effect_cycle_time(speed)` |
| pulse_period | `calculate_effect_period(1000ms, speed)` |
| breathe_period | `calculate_effect_period(4000ms, speed)` |
| color_cycle | `calculate_effect_period(10000ms, speed)` |

---

## 8. Boot Sequence

### State Machine

```
BOOT_WAITING_WIFI ──────────────────────┐
       │                                │
       ▼                                │
BOOT_CAPTIVE_PORTAL ◄───────────────────┤
       │                                │
       ▼                                │
BOOT_WAITING_TIME_SYNC ─────────────────┤
       │                                │
       ▼                                │
BOOT_TRANSITION_TO_TIME                 │
       │                                │
       ▼                                │
BOOT_COMPLETE ──────────────────────────┘
```

### Boot Display

| State | Ring Color | Matrix |
|-------|-----------|--------|
| WAITING_WIFI | Blue | "42" rainbow |
| WAITING_TIME_SYNC | Green | "42" rainbow |
| CAPTIVE_PORTAL | Orange | "42" rainbow |
| TRANSITION | Fading | Crossfade to time |

---

## 9. Architecture Overview

### Component Relationships

```
                    ┌─────────────────┐
                    │    ESPHome      │
                    │   Framework     │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
              ▼              ▼              ▼
      ┌───────────┐  ┌───────────┐  ┌───────────┐
      │  time::   │  │  light::  │  │   wifi::  │
      │ RealTime  │  │Addressable│  │  WiFi     │
      │  Clock    │  │   Light   │  │ Component │
      └─────┬─────┘  └─────┬─────┘  └─────┬─────┘
            │              │              │
            └──────────────┼──────────────┘
                           │
                    ┌──────▼──────┐
                    │  WordClock  │
                    │  Component  │
                    └──────┬──────┘
                           │
       ┌───────────────────┼───────────────────┐
       │                   │                   │
       ▼                   ▼                   ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│LanguageBase │    │ effects.cpp │    │ color_utils │
│ (French/UK) │    │ (rendering) │    │ (HSV cache) │
└─────────────┘    └─────────────┘    └─────────────┘
```

### Data Flow

1. **Time Update**: `loop()` → `compute_active_leds()` → Language → LED arrays
2. **Rendering**: `update_display()` → `apply_light_colors()` → Sub-methods → LED strip
3. **Transitions**: `detect_led_changes()` → Fade states → Progressive blend

---

## Contributing

When making changes:

1. Follow snake_case naming convention
2. Write all comments and variable names **in English**
3. Use constants from `wordclock_config.h::config` and `::defaults`
4. Validate LED indices with `is_excluded_led()`
5. Use `get_second_leds()` for seconds ring access (O(1))
6. Use `StringPool::instance().intern()` for word map keys
7. Test boot sequence and all effects
8. Update this guide to reflect any new behavior

---

## 10. Performance Optimizations

### Memory Optimization

#### Seconds Ring: Array-based Storage

The seconds ring uses `std::array<std::vector<int>, 60>` (240 bytes) for O(1) access:

```cpp
// Direct index access
const auto& leds = get_second_leds(second);
```

- Gaps at positions 0 and 30 are empty vectors
- No string key overhead

#### String Interning (StringPool)

All word strings are stored once in a global pool, and maps use `size_t` indices:

```cpp
// Usage in language files:
auto& pool = StringPool::instance();
ledsarray_start[pool.intern("il")] = {17, 18};
ledsarray_hours[pool.intern("heure")] = {88, 89, 90, 91, 92};
```

Memory savings: ~35-40KB RAM

### CPU Optimization

#### LED Type Index: O(1) Lookup

A 256-byte array provides instant LED type lookup:

```cpp
// Direct lookup instead of iteration
return led_type_index_[led_index];
```

The array is updated after `compute_active_leds()`.

#### HSV Cache
Pre-computed 360-color lookup table for HSV→RGB conversion.
- Rebuilt only when saturation or value changes
- ~10-15% CPU reduction in rainbow effects

#### Adaptive FPS
Dynamic update rate based on visual activity:

| Activity | FPS | Interval |
|----------|-----|----------|
| Effects active + changing | 50 | 20ms |
| Fading only | 20 | 50ms |
| Static display | 10 | 100ms |

```cpp
// Automatic adjustment in loop()
if (adaptive_fps_.should_update(current_millis)) {
  update_display();
}
```

### Performance Metrics

| Metric | Typical Value |
|--------|---------------|
| RAM Usage | ~45-60% |
| CPU Idle | ~8-10% |
| CPU with Effects | ~20-25% |
| FPS Range | 10-50 (adaptive) |

### Memory Layout Summary

```
Component                    Size
─────────────────────────────────
StringPool (all strings)     ~5KB
IndexedLedMap (start)        ~80 bytes
IndexedLedMap (hours)        ~60 bytes
IndexedLedMap (minutes)      ~100 bytes
IndexedLedMap (misc)         ~20 bytes
seconds_ring_leds_ array     240 bytes
led_type_index_ array        256 bytes
Active LED vectors (4)       ~1KB max
─────────────────────────────────
Total                        ~6.5KB
```

### Monitoring in Logs

The debug log output includes FPS interval:
```
[D][wordclock:500]: 14:32:45 [FR] W:15 S:1 | 2.34W | RAM:48.2% | 20ms
```

---

## 11. Advanced Topics

### Custom Effect Implementation

To add a new effect type:

1. **Add to enum** in `wordclock.h`:
```cpp
enum EffectType {
  EFFECT_NONE = 0,
  EFFECT_RAINBOW = 1,
  EFFECT_PULSE = 2,
  EFFECT_BREATHE = 3,
  EFFECT_COLOR_CYCLE = 4,
  EFFECT_YOUR_EFFECT = 5  // Add here
};
```

2. **Implement in effects.cpp**:
```cpp
case EFFECT_YOUR_EFFECT: {
  for (size_t i = 0; i < leds.size(); i++) {
    int led = leds[i];
    float phase = fmod((float)params.now_ms / 1000.0f, 1.0f);
    // Your effect logic here
    Color color = your_effect_calculation(i, phase, params);
    (*output)[led] = color;
  }
  break;
}
```

3. **Add to select options** in `select/__init__.py`

### Vector Pool Pattern

The component uses vector pooling to avoid allocations:

```cpp
// LedVectorPool provides reusable vectors
class LedVectorPool {
  std::vector<std::vector<int>> pool_;
  size_t next_free_{0};
  
  std::vector<int>& acquire();  // Get next vector
  void reset();                  // Reset all (keep capacity)
};

// Usage each frame:
led_pool_.reset();                        // Clear all vectors
active_hours_leds_ = led_pool_.acquire(); // Reuse pre-allocated
// ... fill vector ...
// Next frame: same vectors reused, no malloc/free
```

Benefits:
- No heap fragmentation over time
- Predictable memory usage
- Faster than malloc/free each frame

### Template-Based Registration

Generic component registration reduces code duplication:

```cpp
template<typename T>
void register_component_by_type(T* component, LightType type,
                                 T** hours, T** minutes, 
                                 T** seconds, T** background) {
  switch (type) {
    case LIGHT_HOURS:    *hours = component; break;
    case LIGHT_MINUTES:  *minutes = component; break;
    case LIGHT_SECONDS:  *seconds = component; break;
    case LIGHT_BACKGROUND: *background = component; break;
  }
}

// Used for:
register_component_by_type(light, type, &hours_light_, ...);
register_component_by_type(state, type, &hours_light_state_, ...);
```

### String Pool Internals

Word strings are stored once in memory:

```cpp
// First call: stores string, returns index
StringPool::intern("heure")  → index 42

// Second call: finds existing, returns same index  
StringPool::intern("heure")  → index 42

// Map uses index instead of string
ledsarray_hours_[42] = {88, 89, 90, 91, 92};
```

Lookup complexity:
- Hash map: string → index (O(1) average)
- Vector: index → string (O(1))
- Memory: one copy per unique string (~5KB total)

### Adaptive FPS Algorithm

```
Visual Change Rate (exponential moving average):
  rate = rate × 0.9 + new_intensity × 0.1

FPS Selection:
  if rate ≥ 50%  → 50 FPS (20ms interval)
  if rate ≥ 10%  → 20 FPS (50ms interval)
  else           → 10 FPS (100ms interval)

Change Detection (per frame):
  1. Sum RGB differences for each LED
  2. Normalize to 0-1 range
  3. Apply exponential smoothing
```

### Factory Reset Implementation

Thread-safe reset with arrays:

```cpp
// 1. Disable updates via flag
updates_enabled_ = false;
delay(50);  // Wait for loop to finish

// 2. Reset using lambdas and arrays
auto reset_number = [](WordClockNumber* num, float value) {
  if (num) {
    auto call = num->make_call();
    call.set_value(value);
    call.perform();
  }
};

// Array-based reset (no repetitive if-blocks)
reset_number(number_components_[NUM_WORDS_FADE_IN], 0.3f);
reset_number(number_components_[NUM_WORDS_FADE_OUT], 1.0f);
// ...

// 3. Re-enable updates
updates_enabled_ = true;
```

### Logging Levels

| Level | Usage | Example |
|-------|-------|---------|
| LOGE | Critical errors | "No time sync, rebooting" |
| LOGW | Warnings | "LED mapping not found" |
| LOGI | Important events | "Language changed", "Factory reset" |
| LOGD | Debug (every second) | Time, power, RAM, FPS stats |

### Troubleshooting

| Symptom | Possible Cause | Solution |
|---------|----------------|----------|
| High RAM usage | Vector pool growing | Check `led_pool_.pool_size()` |
| Sluggish effects | FPS stuck at 10 | Verify effects are enabled |
| Missing words | LED mapping error | Check ESP logs for warnings |
| Boot timeout | No NTP sync | Verify WiFi and NTP servers |
| Flickering | Effect speed too high | Reduce effect_speed below 90% |
| Unresponsive | Factory reset stuck | Check updates_enabled_ flag |

---