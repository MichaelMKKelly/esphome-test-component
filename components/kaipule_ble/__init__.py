"""Support for Kaipule iSensor BLE devices.

Parses manufacturer-specific advertising data from Kaipule iSensor BLE
sensors (non-connectable ADV_NON_CONN_IND broadcasts) and exposes the
decoded data to Home Assistant via the ``kaipule_ble`` binary_sensor
and sensor platforms.

Protocol reference (see protocol_information/):
  - "Kaipule BLE Comm Protocol.pdf"  (V1.0, 2015-10-08)
  - "Kaipule iSensor Profile.pdf"    (V1.0, 2015-11-18)

Advertising layout (per iSensor Profile):
  02 01 06            GAP flags
  09 08 "iSensor "    shortened local name (8 chars + trailing space)
  09 FF <8..12 bytes> manufacturer specific data = sensor frame

Sensor frame (8 bytes base):
  [0]    firmware version
  [1..3] device ID
  [4]    type ID   (bit7 bi-dir, bit6 periodic report, bits5-0 type code)
  [5]    event data (bit3 heartbeat, bit2 low-voltage, bit1 alarm, bit0 tamper)
  [6]    control data (frame ID)
  [7]    checksum = sum(bytes[0..6]) & 0xFF

Temperature/humidity sensors (type code 0x0A) extend the frame to 12
bytes, appending: temp integral, temp decimal, humidity integral,
humidity decimal, then the checksum over all preceding bytes.
Negative temperatures are transmitted as positive values; the actual
value is display - 256.
"""
import esphome.codegen as cg
from esphome.components import ble_device_base
import esphome.config_validation as cv
from esphome.const import CONF_ID

AUTO_LOAD = ["ble_device_base", "binary_sensor", "sensor"]

kaipule_ns = cg.esphome_ns.namespace("kaipule_ble")
KaipuleBLE = kaipule_ns.class_("KaipuleBLE", ble_device_base.ESPBTDeviceListener)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(KaipuleBLE),
    }
).extend(ble_device_base.BLE_DEVICE_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await ble_device_base.register_ble_device(var, config)
