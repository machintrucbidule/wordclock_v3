# WordClock v3 – ESPHome External Component

**Word Clock** driven by ESP32, based on an **ESPHome external component**. This project provides a reusable component (light, switch, number, select, button) to drive an LED matrix displaying the time in words (FR / EN UK), with full Home Assistant integration.

---

## Features

### Core features

* Time displayed using words (Word Clock concept)
* Fixed LED matrix layout (letter grid)
* Language-aware word mapping
* ESP32 support (tested on ESP32-C6)
* Full Home Assistant integration via ESPHome
* Implemented as a **reusable external ESPHome component**

### Supported languages

* **French** (`fr`)
* **English (UK)** (`en_uk`)

Each language has its own word layout, grammar rules, and special cases (minutes, transitions, hour increments).

### Optional features

* Automatic brightness control using an ambient light sensor
* Presence-based behavior using an LD2410 radar

> Optional features are disabled by default and must be explicitly configured.

---

## Repository structure

The project is split between **public configuration**, **language logic**, and **internal helpers**.

```text
wordclock_v3/
├── README.md
├── DEVELOPER_GUIDE.md
└── components/
    └── wordclock/
        ├── __init__.py
        ├── wordclock.cpp        # Core logic
        ├── wordclock.h
        ├── wordclock_config.h  # ESPHome config schema
        ├── effects.cpp         # LED effects
        ├── color_utils.h
        ├── led_utils.h
        ├── string_pool.h
        ├── language_base.h     # Common language interface
        ├── lang_french.h       # French matrix + rules
        ├── lang_english_uk.h   # English UK matrix + rules
        ├── light/              # Light platform bindings
        ├── switch/             # Switch entities
        ├── number/             # Number entities
        ├── select/             # Select entities
        └── button/             # Button entities
```

The component exposes ESPHome entities while keeping all display logic internal.

---

## Installation

### 1. Add the external component

In your ESPHome YAML file:

```yaml
external_components:
  - source: github://machintrucbidule/wordclock_v3
    components: [wordclock]
```

---

## ESPHome YAML – Minimal example

```yaml
substitutions:
  device_name: wordclock3
  friendly_name: WordClock

esphome:
  name: ${device_name}

esp32:
  board: esp32-c6-devkitc-1

logger:

api:

ota:

wifi:
  ap:
    ssid: "${friendly_name} Setup"

captive_portal:

# Time synchronization
time:
  - platform: sntp
    id: sntp_time
    timezone: Europe/Paris

# ======================
# LED Strip
# ======================
light:
  - platform: esp32_rmt_led_strip
    id: wordclock_leds
    pin: GPIO6
    num_leds: 256
    rgb_order: GRB
    rmt_channel: 0
    chipset: ws2812

# ======================
# WordClock
# ======================
wordclock:
  light: wordclock_leds
  time_id: sntp_time
  language: fr
```

---

## Matrix concept

The WordClock uses a **fixed letter matrix** where each LED corresponds to a letter position.

* The matrix layout is **language-specific**
* Each language defines:

  * the letter grid
  * word positions (start index + length)
  * grammatical rules

### French matrix behavior (example)

* Uses constructs like `IL EST`, `MOINS`, `ET QUART`
* Hour increments when using `MOINS`
* Special handling for `MIDI` / `MINUIT`

### English (UK) matrix behavior (example)

* Uses `IT IS`, `PAST`, `TO`
* Next hour is used when minutes >= 35 (`TWENTY FIVE TO SIX`)
* No AM/PM, purely 24h-based logic

All this logic is embedded in the language files and completely transparent to the user.

---

## WordClock configuration

### Core options

| Option     | Type     | Description                | Values        |
| ---------- | -------- | -------------------------- | ------------- |
| `light`    | required | LED strip used for display | `light` id    |
| `time_id`  | required | Time source                | `time` id     |
| `language` | optional | Display language           | `fr`, `en_uk` |

### Optional options

| Option              | Type          | Description         |
| ------------------- | ------------- | ------------------- |
| `brightness_sensor` | sensor        | Ambient light input |
| `presence_sensor`   | binary_sensor | Presence detection  |

### Behavior options

Depending on configuration, the WordClock can:

* Dim or brighten automatically
* Turn off or reduce brightness when no presence is detected
* React instantly to time changes (minute resolution)

---------|-------------|
| `light` | LED entity used for display |
| `time_id` | ESPHome time source |
| `language` | `fr` or `en_uk` |

---

## Adding an ambient light sensor (optional)

Example using an ADC sensor:

```yaml
sensor:
  - platform: adc
    pin: GPIO4
    id: ambient_light
    update_interval: 5s

wordclock:
  light: wordclock_leds
  time_id: sntp_time
  language: fr
  brightness_sensor: ambient_light
```

---

## Adding an LD2410 presence radar (optional)

```yaml
uart:
  rx_pin: GPIO20
  tx_pin: GPIO21
  baud_rate: 256000

ld2410:

binary_sensor:
  - platform: ld2410
    has_target:
      name: "Presence"
      id: presence

wordclock:
  light: wordclock_leds
  time_id: sntp_time
  language: fr
  presence_sensor: presence
```

---

## Home Assistant integration

All entities exposed by the component (light, switch, number, select, button) are automatically available in Home Assistant via the ESPHome API.

---

## Development notes

* Component follows the standard ESPHome `components/` structure
* Compatible with local builds and GitHub external components
* No ESPHome patching required

See **DEVELOPER_GUIDE.md** for internal implementation details.

---

## License

Open-source project – license to be defined (MIT recommended).

---

## Status

✔ Functional
✔ Ready for GitHub
✔ Reusable across multiple ESPHome projects
