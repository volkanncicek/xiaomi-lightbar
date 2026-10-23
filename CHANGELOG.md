# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Releases are created by CI.** A `v*` tag is checked against the version in `pyproject.toml`, compiled, and released with this file's section as its notes.
- **Dependabot** keeps the GitHub Actions and `uv.lock` current, weekly.

## [0.1.1] - 2026-10-23

### Fixed

- **RF24 is constrained to `^1.6.0`.** The component requested RF24 with no version, so every build took whatever was newest; 1.6 is the first line with ESP-IDF support, and anything older fails trying to build the Raspberry Pi `pigpio` backend.

### Added

- **CI compile check.** Every push validates and compiles `example/compile-test.yaml` against the ESPHome version pinned in `uv.lock`.

### Changed

- **README** gains a development section, the tested ESPHome version, and a pointer to lightbar2mqtt for sniffing the original remote's serial.

## [0.1.0] - 2026-10-04

First release.

### Added

- **ESPHome light component** for the Xiaomi MJGJD01YL monitor light bar over an nRF24 radio: on/off, brightness and colour temperature from Home Assistant over the native API, pairing by serial on channel 68 at 2 Mbps.
- **Coalesced radio sends.** Commands go out from `loop()` rather than `write_state()`, so dragging a slider sends one final command instead of a burst per event.
- **Standalone `XiaomiLightbar` C++ core** that depends only on RF24 and works in any Arduino or PlatformIO project.

[Unreleased]: https://github.com/volkanncicek/xiaomi-lightbar/compare/v0.1.1...HEAD
[0.1.1]: https://github.com/volkanncicek/xiaomi-lightbar/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/volkanncicek/xiaomi-lightbar/releases/tag/v0.1.0
