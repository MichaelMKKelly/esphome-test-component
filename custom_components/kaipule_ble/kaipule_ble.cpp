#include "kaipule_ble.h"

#include <cstdio>

namespace esphome {
namespace kaipule_ble {

// Type code (bits 5-0 of the type ID byte) for the temperature &
// humidity sensor, per "Kaipule BLE Comm Protocol.pdf" section 3.
static constexpr uint8_t TYPE_TEMPERATURE = 0x0A;

static const char *TAG = "kaipule_ble";

// ---------------------------------------------------------------------------
// KaipuleBLE
// ---------------------------------------------------------------------------

void KaipuleBLE::add_subscriber(const std::string &mac, std::function<void(const DeviceState &)> callback) {
  this->subscribers_.emplace_back(mac, std::move(callback));
}

std::string KaipuleBLE::bytes_to_hex(const std::vector<uint8_t> &data) {
  std::string result;
  result.reserve(data.size() * 3);
  char buf[4];
  for (auto b : data) {
    snprintf(buf, sizeof(buf), "%02X ", b);
    result += buf;
  }
  return result;
}

bool KaipuleBLE::parse_device(const ble_device_base::ESPBTDevice &device) {
  char mac_buf[ble_device_base::ESPBTDevice::MAC_ADDRESS_PRETTY_BUFFER_SIZE];
  const char *mac = device.address_str_to(mac_buf);

  for (const auto &sd : device.get_manufacturer_datas()) {
    if (!sd.uuid.is_set())
      continue;

    // ESPHome strips the first 2 bytes of raw manufacturer data and
    // stores them as a "company ID" UUID. Kaipule frames carry NO
    // company ID, so the first two frame bytes live in
    // sd.uuid.uuid16() and the rest in sd.data. Reconstruct the full
    // frame before validating the checksum.
    const uint16_t company_id = sd.uuid.uuid16();
    std::vector<uint8_t> frame;
    frame.reserve(sd.data.size() + 2);
    frame.push_back(company_id & 0xFF);
    frame.push_back((company_id >> 8) & 0xFF);
    frame.insert(frame.end(), sd.data.begin(), sd.data.end());

    // Log every reconstructed frame. This is the discovery mechanism:
    // watch the logs, note the MAC + hex payload of your sensors, then
    // add them to the config.
    ESP_LOGD(TAG, "adv %s name='%s' rssi=%d mfg=[%s]", mac, device.get_name().c_str(), (int) device.get_rssi(),
             bytes_to_hex(frame).c_str());

    DeviceState state;
    if (!this->parse_frame(frame, state))
      continue;

    state.rssi = static_cast<int8_t>(device.get_rssi());
    state.seen = true;

    this->states_[mac] = state;

    for (auto &sub : this->subscribers_) {
      if (sub.first == mac)
        sub.second(state);
    }
  }

  // Do not claim the advertisement: other listeners (e.g.
  // bluetooth_proxy) must still see it.
  return false;
}

bool KaipuleBLE::parse_frame(const std::vector<uint8_t> &data, DeviceState &state) {
  if (data.size() < 8)
    return false;

  const uint8_t type_id = data[4];
  const uint8_t event_data = data[5];
  const uint8_t type_code = type_id & 0x3F;

  // Base frame is 8 bytes; temperature/humidity sensors extend it to
  // 12 bytes (4 extra payload bytes + checksum over the full frame).
  size_t frame_len = 8;
  if (type_code == TYPE_TEMPERATURE && data.size() >= 12)
    frame_len = 12;

  // Checksum = sum of all preceding bytes (mod 256).
  uint16_t sum = 0;
  for (size_t i = 0; i + 1 < frame_len; i++)
    sum += data[i];
  if (sum != data[frame_len - 1])
    return false;

  state.type_id = type_id;
  state.alarm = (event_data & 0x02) != 0;
  state.tamper = (event_data & 0x01) != 0;
  state.low_battery = (event_data & 0x04) != 0;

  // Temperature/humidity payload (type code 0x0A only):
  //   [7] temp integral, [8] temp decimal (tenths),
  //   [9] humidity integral, [10] humidity decimal (tenths).
  // Negative temperatures are sent as positive values: actual = display - 256.
  if (type_code == TYPE_TEMPERATURE && frame_len >= 12) {
    const uint8_t temp_int = data[7];
    const uint8_t temp_dec = data[8];
    const uint8_t hum_int = data[9];
    const uint8_t hum_dec = data[10];

    float temp = temp_int + temp_dec / 10.0f;
    if (temp_int >= 200)
      temp -= 256.0f;
    state.temperature = temp;
    state.humidity = hum_int + hum_dec / 10.0f;
  }

  return true;
}

// ---------------------------------------------------------------------------
// KaipuleBinarySensor
// ---------------------------------------------------------------------------

void KaipuleBinarySensor::setup() {
  auto *parent = id(kaipule);
  parent->add_subscriber(this->mac_, [this](const DeviceState &state) {
    bool value = false;
    if (this->type_ == "alarm")
      value = state.alarm;
    else if (this->type_ == "tamper")
      value = state.tamper;
    else if (this->type_ == "low_battery")
      value = state.low_battery;
    this->publish_state(value);
  });
}

// ---------------------------------------------------------------------------
// KaipuleSensor
// ---------------------------------------------------------------------------

void KaipuleSensor::setup() {
  if (this->type_ == "temperature")
    this->set_unit_of_measurement("°C");
  else if (this->type_ == "humidity")
    this->set_unit_of_measurement("%");
  else if (this->type_ == "rssi")
    this->set_unit_of_measurement("dBm");

  auto *parent = id(kaipule);
  parent->add_subscriber(this->mac_, [this](const DeviceState &state) {
    float value = NAN;
    if (this->type_ == "temperature")
      value = state.temperature;
    else if (this->type_ == "humidity")
      value = state.humidity;
    else if (this->type_ == "rssi")
      value = state.rssi;
    this->publish_state(value);
  });
}

}  // namespace kaipule_ble
}  // namespace esphome
