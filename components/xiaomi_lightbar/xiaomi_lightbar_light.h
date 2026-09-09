#pragma once
#include <cmath>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/light/light_output.h"
#include "xiaomi_lightbar.h"

namespace esphome {
namespace xiaomi_lightbar {

// ESPHome color-temperature light backed by the radio-only XiaomiLightbar driver.
// The bar reports no state, so this tracks its own belief (assumed state): on/off
// is a blind toggle, and only changed brightness/temperature levels are resent.
class XiaomiLightbarLight : public Component, public light::LightOutput {
 public:
  void set_serial(uint32_t serial) { serial_ = serial; }
  void set_pins(uint8_t ce, uint8_t csn, uint8_t sck, uint8_t miso, uint8_t mosi) {
    pins_ = {ce, csn, sck, miso, mosi};
  }
  void set_cold_mireds(float mireds) { cold_mireds_ = mireds; }
  void set_warm_mireds(float mireds) { warm_mireds_ = mireds; }

  void setup() override {
    bar_ = new XiaomiLightbar(serial_, pins_);
    bar_->begin();
  }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
    traits.set_min_mireds(cold_mireds_);
    traits.set_max_mireds(warm_mireds_);
    return traits;
  }

  void write_state(light::LightState *state) override {
    auto values = state->current_values;
    bool on = values.is_on();
    uint8_t level = static_cast<uint8_t>(lroundf(values.get_brightness() * XiaomiLightbar::max_level));
    // HA mireds: low = cold, high = warm. The bar's level 0 = warmest, 15 = coolest,
    // so invert the fraction to map the two.
    float span = warm_mireds_ - cold_mireds_;
    float fraction = span > 0.0f ? (values.get_color_temperature() - cold_mireds_) / span : 0.0f;
    uint8_t temp = static_cast<uint8_t>(lroundf((1.0f - fraction) * XiaomiLightbar::max_level));

    if (!initialized_) {
      // Adopt whatever ESPHome restored as our belief, but never actuate on boot:
      // a blind toggle here would flip a lamp the ESP32 reboot should not touch.
      is_on_ = on;
      brightness_ = level;
      color_temp_ = temp;
      synced_ = true;
      initialized_ = true;
      ESP_LOGD("xlb", "adopt boot state on=%d level=%d temp=%d (no actuation)", on, level, temp);
      return;
    }

    // Only record the target; loop() does the radio sends. Each send blocks ~200ms
    // (the 20x burst) and a single HA change can imply up to 5 sends, so sending here
    // would stall the main loop for ~1s and a slider drag would queue dozens. Storing
    // the latest target lets loop() coalesce a drag to one final send per property.
    target_on_ = on;
    target_brightness_ = level;
    target_color_temp_ = temp;
    have_target_ = true;
    ESP_LOGD("xlb", "target on=%d level=%d temp=%d", on, level, temp);
  }

  // Drain one property difference per pass so the burst never blocks more than one
  // send at a time; write_state may overwrite the target between passes (drag coalesce).
  void loop() override {
    if (!have_target_ || bar_ == nullptr) return;

    if (target_on_ != is_on_) {
      bar_->toggle();
      is_on_ = target_on_;
      return;
    }
    if (!target_on_) {  // off: nothing to sync while dark
      have_target_ = false;
      return;
    }
    if (target_brightness_ != brightness_ || !synced_) {
      bar_->set_brightness(target_brightness_);
      brightness_ = target_brightness_;
      ESP_LOGD("xlb", "sent brightness=%d", brightness_);
      return;
    }
    if (target_color_temp_ != color_temp_ || !synced_) {
      bar_->set_color_temp(target_color_temp_);
      color_temp_ = target_color_temp_;
      ESP_LOGD("xlb", "sent color_temp=%d", color_temp_);
      return;
    }
    synced_ = true;
    have_target_ = false;
  }

 protected:
  uint32_t serial_{0};
  XiaomiLightbar::Pins pins_{};
  float cold_mireds_{153};
  float warm_mireds_{370};
  XiaomiLightbar *bar_{nullptr};
  bool is_on_{true};  // the bar powers on ON by default; best assumed-state guess
  bool initialized_{false};
  bool synced_{false};
  uint8_t brightness_{0};
  uint8_t color_temp_{0};
  bool target_on_{true};
  bool have_target_{false};
  uint8_t target_brightness_{0};
  uint8_t target_color_temp_{0};
};

}  // namespace xiaomi_lightbar
}  // namespace esphome
