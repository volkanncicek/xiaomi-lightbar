#pragma once
#include <RF24.h>

// Control-only driver for the Xiaomi Mi Computer Monitor Light Bar (MJGJD01YL),
// which listens on a 2.4 GHz nRF24-compatible link. The bar never reports state
// back, so on/off is a blind toggle. Protocol reverse-engineered by lamperez.
class XiaomiLightbar {
 public:
  struct Pins {
    uint8_t ce, csn, sck, miso, mosi;
  };

  static constexpr uint8_t max_level = 15;

  XiaomiLightbar(uint32_t serial, const Pins &pins);

  void begin();
  void pair();                        // bind the bar to this serial; power-cycle the bar first
  void toggle();
  void set_brightness(uint8_t level);  // absolute, 0..15
  void set_color_temp(uint8_t level);  // absolute, 0..15, 0 = coolest
  void brighter(uint8_t steps = 1);
  void dimmer(uint8_t steps = 1);
  void warmer(uint8_t steps = 1);
  void cooler(uint8_t steps = 1);

 private:
  enum class Command : uint8_t {
    TOGGLE = 1, COOLER = 2, WARMER = 3, BRIGHTER = 4, DIMMER = 5, RESET = 6
  };
  void send(Command command, uint8_t steps = 0);
  static uint16_t crc16(const uint8_t *data, uint8_t len);

  RF24 radio_;
  const uint32_t serial_;
  const Pins pins_;
  uint8_t sequence_ = 0;
};
