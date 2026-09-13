"""Sensor platform for Kaipule iSensor BLE devices.

Exposes temperature, humidity (for type 0x0A temperature/humidity
sensors) and the received signal strength (RSSI) of any Kaipule
iSensor device.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_TYPE

from . import kaipule_ns

DEPENDENCIES = ["kaipule_ble"]

KaipuleSensor = kaipule_ns.class_("KaipuleSensor", sensor.Sensor, cg.Component)

SENSOR_TYPES = {
    "temperature": "temperature",
    "humidity": "humidity",
    "rssi": "rssi",
}

CONFIG_SCHEMA = sensor.sensor_schema(
    KaipuleSensor,
    accuracy_decimals=1,
).extend(
    {
        cv.Required("device_mac"): cv.mac_address,
        cv.Required(CONF_TYPE): cv.enum(SENSOR_TYPES, lower=True),
    }
)


async def to_code(config):
    var = await cg.new_variable(config)
    await cg.register_component(var, config)
    await cg.add(var.set_mac(config["device_mac"]))
    await cg.add(var.set_type(config[CONF_TYPE]))
