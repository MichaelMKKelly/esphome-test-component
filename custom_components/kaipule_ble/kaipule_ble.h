#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/components/ble_device_base/ble_device.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace kaipule_ble {

/// Parsed state of a Kaipule iSensor BLE device.
struct DeviceState {
  bool alarm = false;
  bool tamper = false;
  bool low_battery = false;
  float temperature = NAN;
  float humidity = NAN;
  int8_t rssi = 0;
  uint8_t type_id = 0;
  bool seen = false;
};

/// Parsed-advertisement listener: parses Kaipule iSensor frames from
/// manufacturer data and fans the decoded state out to registered
/// subscribers (the binary_sensor / sensor platforms).
class KaipuleBLE : public ble_device_base::ESPBTDeviceListener {
 public:
  /// Register a callback invoked whenever data for \p mac is received.
  void add_subscriber(const std::string &mac, std::function<void(const DeviceState &)> callback);

  /// Called by the BLE tracker for every advertisement.
  bool parse_device(const ble_device_base::ESPBTDevice &device) override;

 protected:
  bool parse_frame(const std::vector<uint8_t> &data, DeviceState &state);
  static std::string bytes_to_hex(const std::vector<uint8_t> &data);

  std::map<std::string, DeviceState> states_;
  std::vector<std::pair<std::string, std::function<void(const DeviceState &)>>> subscribers_;
};

/// Binary sensor platform (alarm / tamper / low battery).
class KaipuleBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void set_mac(const std::string &mac) { this->mac_ = mac; }
  void set_type(const std::string &type) { this->type_ = type; }

 protected:
  std::string mac_;
  std::string type_;
};

/// Sensor platform (temperature / humidity / rssi).
class KaipuleSensor : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void set_mac(const std::string &mac) { this->mac_ = mac; }
  void set_type(const std::string &type) { this->type_ = type; }

 protected:
  std::string mac_;
  std::string type_;
};

}  // namespace kaipule_ble
}  // namespace esphome
