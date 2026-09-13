"""Sensor platform for Kaipule iSensor BLE devices.

Exposes temperature, humidity (for type 0x0A temperature/humidity
sensors) and the received signal strength (RSSI) of any Kaipule
iSensor device.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_TYPE, CONF_UNIT_OF_MEASUREMENT

from . import KaipuleBLE, kaipule_ns

DEPENDENCIES = ["kaipule_ble"]

KaipuleSensor = kaipule_ns.class_("KaipuleSensor", sensor.Sensor, cg.Component)

SENSOR_TYPES = {
    "temperature": "temperature",
    "humidity": "humidity",
    "rssi": "rssi",
}

# Unit of measurement per sensor type. Applied in to_code via the entity
# string pool (the 2026.8.x Sensor class has no C++ set_unit_of_measurement).
UNIT_BY_TYPE = {
    "temperature": "°C",
    "humidity": "%",
    "rssi": "dBm",
}

CONFIG_SCHEMA = sensor.sensor_schema(
    KaipuleSensor,
    accuracy_decimals=1,
).extend(
    {
        cv.Required("kaipule_ble"): cv.use_id(KaipuleBLE),
        cv.Required("device_mac"): cv.mac_address,
        cv.Required(CONF_TYPE): cv.enum(SENSOR_TYPES, lower=True),
    }
)


async def to_code(config):
    # Set the unit before register_sensor so setup_unit_of_measurement
    # (invoked by register_sensor) picks it up from the config.
    config[CONF_UNIT_OF_MEASUREMENT] = UNIT_BY_TYPE[config[CONF_TYPE]]
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
    parent = await cg.get_variable(config["kaipule_ble"])
    cg.add(var.set_parent(parent))
    cg.add(var.set_mac(config["device_mac"]))
    cg.add(var.set_type(config[CONF_TYPE]))
