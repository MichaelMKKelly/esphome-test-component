"""Binary sensor platform for Kaipule iSensor BLE devices.

Exposes the alarm, tamper and low-battery status bits from the
event-data byte of the Kaipule iSensor advertising frame.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_TYPE

from . import kaipule_ns

DEPENDENCIES = ["kaipule_ble"]

KaipuleBinarySensor = kaipule_ns.class_("KaipuleBinarySensor", binary_sensor.BinarySensor, cg.Component)

BINARY_SENSOR_TYPES = {
    "alarm": "alarm",
    "tamper": "tamper",
    "low_battery": "low_battery",
}

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(KaipuleBinarySensor).extend(
    {
        cv.Required("device_mac"): cv.mac_address,
        cv.Required(CONF_TYPE): cv.enum(BINARY_SENSOR_TYPES, lower=True),
    }
)


async def to_code(config):
    var = await cg.new_variable(config)
    await cg.register_component(var, config)
    await cg.add(var.set_mac(config["device_mac"]))
    await cg.add(var.set_type(config[CONF_TYPE]))
