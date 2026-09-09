#include "xiaomi_lightbar.h"

#include <SPI.h>
#include <string.h>

namespace {
constexpr uint8_t PREAMBLE[8] = {0x53, 0x39, 0x14, 0xDD, 0x1C, 0x49, 0x34, 0x12};
constexpr uint64_t WRITE_ADDRESS = 0x5555555555;
constexpr uint8_t CHANNEL = 68;
constexpr uint8_t PACKET_SIZE = 17;
constexpr uint8_t REPEAT = 20;  // the bar needs the burst; it dedupes by sequence byte
}  // namespace

XiaomiLightbar::XiaomiLightbar(uint32_t serial, const Pins &pins)
    : radio_(pins.ce, pins.csn), serial_(serial), pins_(pins) {}

void XiaomiLightbar::begin() {
  SPI.begin(pins_.sck, pins_.miso, pins_.mosi);
  radio_.begin();
  radio_.setChannel(CHANNEL);
  radio_.setDataRate(RF24_2MBPS);
  radio_.setPALevel(RF24_PA_LOW);
  radio_.disableCRC();
  radio_.disableDynamicPayloads();
  radio_.setAutoAck(false);
  radio_.setPayloadSize(PACKET_SIZE);
  radio_.openWritingPipe(WRITE_ADDRESS);
  radio_.stopListening();
}

void XiaomiLightbar::send(Command command, uint8_t steps) {
  uint8_t packet[PACKET_SIZE] = {0};
  memcpy(packet, PREAMBLE, sizeof(PREAMBLE));
  packet[8] = serial_ >> 16;
  packet[9] = serial_ >> 8;
  packet[10] = serial_;
  packet[11] = 0xFF;
  packet[12] = ++sequence_;
  packet[13] = static_cast<uint8_t>(command);
  packet[14] = steps;
  const uint16_t crc = crc16(packet, PACKET_SIZE - 2);
  packet[15] = crc >> 8;
  packet[16] = crc;

  radio_.stopListening();
  for (uint8_t i = 0; i < REPEAT; i++) {
    radio_.write(packet, PACKET_SIZE);
    delay(10);
  }
}

// CRC16/CCITT variant used by the bar: poly 0x1021, init 0xFFFE, no reflection.
uint16_t XiaomiLightbar::crc16(const uint8_t *data, uint8_t len) {
  uint16_t crc = 0xFFFE;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
  }
  return crc;
}

void XiaomiLightbar::pair() { send(Command::RESET); }
void XiaomiLightbar::toggle() { send(Command::TOGGLE); }
void XiaomiLightbar::brighter(uint8_t steps) { send(Command::BRIGHTER, steps); }
void XiaomiLightbar::dimmer(uint8_t steps) { send(Command::DIMMER, steps); }
void XiaomiLightbar::warmer(uint8_t steps) { send(Command::WARMER, steps); }
void XiaomiLightbar::cooler(uint8_t steps) { send(Command::COOLER, steps); }

// Absolute level from an unknown state: overshoot to the extreme (0xF0, far past
// max_level so it saturates from any current position), then step back in by the
// target. A smaller overshoot fails to reach the extreme and the level lands wrong.
void XiaomiLightbar::set_brightness(uint8_t level) {
  if (level > max_level) level = max_level;
  send(Command::DIMMER, 0xF0);
  send(Command::BRIGHTER, level);
}

void XiaomiLightbar::set_color_temp(uint8_t level) {
  if (level > max_level) level = max_level;
  send(Command::COOLER, 0xF0);
  send(Command::WARMER, level);
}
