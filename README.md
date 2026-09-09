# xiaomi-lightbar

Control the Xiaomi Mi Computer Monitor Light Bar (MJGJD01YL) from Home Assistant, over the bar's native 2.4 GHz nRF24 radio link.

The MJGJD01YL is the non-smart light bar: no WiFi, no Bluetooth, normally driven only by its bundled rotary remote. This project puts an ESP32 plus an nRF24 radio in the middle, speaks the bar's reverse-engineered protocol, and exposes it to Home Assistant as a dimmable, colour-temperature light.

This is, as far as I know, the first ESPHome component for this bar. It also ships a standalone C++ driver you can use without ESPHome.

## Features

- On/off, brightness, and colour temperature from Home Assistant over the ESPHome native API — no MQTT, no cloud.
- A standalone `XiaomiLightbar` C++ core (RF24 only) usable in any Arduino/PlatformIO project, independent of ESPHome.
- OTA updates once flashed.

## What you need

- An ESP32 board.
- An **nRF24L01+** module (see the note below).
- Female-to-female jumper wires (7 for the radio).

### nRF24 module note

Use the **plain nRF24L01+** module (small on-board PCB antenna). Avoid the **+PA/LNA** variant (the larger one with an external antenna): its amplifier overloads the receiver at close range, which is exactly where this project runs (the ESP32 sits next to the bar). Add a `10 µF` capacitor across the module's `VCC`/`GND` for reliability on any variant.

Most cheap modules are Si24R1 clones rather than genuine Nordic parts, which is fine here: this project uses raw packets with no auto-ACK and no dynamic payloads, so the clones' known ACK/DPL incompatibility never comes into play.

### Wiring

nRF24 to ESP32, using the default pins (all configurable):

| nRF24 | ESP32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| CE | GPIO4 |
| CSN | GPIO5 |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| IRQ | not connected |

## Home Assistant setup

Create a device config (`lightbar.yaml`) that pulls in the component and defines the light:

```yaml
esphome:
  name: xiaomi-lightbar

external_components:
  - source: github://volkanncicek/xiaomi-lightbar

esp32:
  board: esp32dev
  framework:
    type: arduino

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
ota:
  - platform: esphome

logger:

light:
  - platform: xiaomi_lightbar
    name: "Desk Lightbar"
    serial: 0xABCDEF
    ce_pin: 4
    csn_pin: 5
    sck_pin: 18
    miso_pin: 19
    mosi_pin: 23
    restore_mode: ALWAYS_ON
```

Put your WiFi credentials in a `secrets.yaml` next to it (see `example/secrets.yaml.example`), then flash once over USB:

```
esphome run lightbar.yaml
```

The device then appears under Home Assistant's ESPHome integration, and later updates go over the air.

### Pairing

The bar obeys a single controller serial. Pick any 24-bit value for `serial:`. To bind the bar to it, power-cycle the bar and, within about 10 seconds, send a pairing (RESET) command; the bar blinks to confirm. Using an arbitrary serial disables the original rotary remote. To keep the remote working alongside Home Assistant, set `serial:` to the remote's own serial instead (see Limitations).

## Configuration

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `serial` | yes | | 24-bit controller serial the bar is paired to (e.g. `0xABCDEF`). |
| `ce_pin`, `csn_pin`, `sck_pin`, `miso_pin`, `mosi_pin` | yes | | nRF24 wiring, as GPIO numbers. |
| `cold_white_color_temperature` | no | `153 mireds` | Coldest colour temperature. |
| `warm_white_color_temperature` | no | `370 mireds` | Warmest colour temperature. |

Plus the standard ESPHome `light` options (`name`, `id`, `restore_mode`, ...).

## Standalone use (without ESPHome)

The `XiaomiLightbar` class under `components/xiaomi_lightbar/` depends only on the RF24 library:

```cpp
#include "xiaomi_lightbar.h"

XiaomiLightbar bar(0xABCDEF, {4, 5, 18, 19, 23});  // ce, csn, sck, miso, mosi

void setup() {
  bar.begin();
  bar.pair();            // power-cycle the bar first
}

void loop() {
  bar.toggle();
  bar.set_brightness(10);   // 0..15
  bar.set_color_temp(3);    // 0..15, 0 = coolest
  delay(5000);
}
```

## How it works

- The bar never reports its state, so on/off is a blind toggle with assumed state; brightness and colour temperature are set absolutely by overshooting to the extreme, then stepping in.
- Radio sends run from `loop()`, not from `write_state()`, so dragging a slider coalesces to a single final command instead of blocking the main loop with a burst per event.
- Each command is transmitted as a 20-write burst; the bar de-duplicates by a sequence byte, so the burst is pure redundancy over the lossy one-way link.

If you build on this, these are the traps that cost time (all handled here):

- RF24 talks over SPI but does not declare it, and ESPHome 2026.2+ disables Arduino libraries by default, so both `SPI` and `RF24` must be added via `cg.add_library`.
- Pin a recent RF24 (1.6.x or newer). Older versions lack ESP-IDF component support and fail to build under ESPHome, trying to compile the Raspberry Pi `pigpio` backend.
- The absolute-set overshoot uses `0xF0`; a smaller value does not force the bar to the extreme and the level lands wrong.

## Limitations

- Control only. The bar sends nothing back, so Home Assistant shows the last command, not the bar's real state; if they drift, toggle once to re-sync.
- The occasional command is dropped over the one-way radio (more so on cheap clone modules and at distance). The bar's own remote has the same trait.
- Keeping the original rotary remote alive alongside Home Assistant needs the remote's own serial, obtained by sniffing it. Reliable sniffing needs clean reception, which a +PA/LNA clone at close range does not provide; a plain module is needed for that.

## Credits

- Protocol reverse-engineering: [lamperez/xiaomi-lightbar-nrf24](https://github.com/lamperez/xiaomi-lightbar-nrf24).
- Reference implementation: [ebinf/lightbar2mqtt](https://github.com/ebinf/lightbar2mqtt).

## License

MIT, see [LICENSE](LICENSE). The RF24 dependency is GPLv2; linking it into your firmware makes the resulting binary GPLv2, even though this project's own source is MIT.
