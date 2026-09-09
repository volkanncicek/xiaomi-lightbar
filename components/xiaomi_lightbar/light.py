import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

CONF_SERIAL = "serial"
CONF_CE_PIN = "ce_pin"
CONF_CSN_PIN = "csn_pin"
CONF_SCK_PIN = "sck_pin"
CONF_MISO_PIN = "miso_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_COLD_WHITE = "cold_white_color_temperature"
CONF_WARM_WHITE = "warm_white_color_temperature"

xiaomi_lightbar_ns = cg.esphome_ns.namespace("xiaomi_lightbar")
XiaomiLightbarLight = xiaomi_lightbar_ns.class_(
    "XiaomiLightbarLight", cg.Component, light.LightOutput
)

_gpio = cv.int_range(min=0, max=48)

CONFIG_SCHEMA = light.LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(XiaomiLightbarLight),
        cv.Required(CONF_SERIAL): cv.hex_int,
        cv.Required(CONF_CE_PIN): _gpio,
        cv.Required(CONF_CSN_PIN): _gpio,
        cv.Required(CONF_SCK_PIN): _gpio,
        cv.Required(CONF_MISO_PIN): _gpio,
        cv.Required(CONF_MOSI_PIN): _gpio,
        cv.Optional(CONF_COLD_WHITE, default="153 mireds"): cv.color_temperature,
        cv.Optional(CONF_WARM_WHITE, default="370 mireds"): cv.color_temperature,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    cg.add(var.set_serial(config[CONF_SERIAL]))
    cg.add(
        var.set_pins(
            config[CONF_CE_PIN],
            config[CONF_CSN_PIN],
            config[CONF_SCK_PIN],
            config[CONF_MISO_PIN],
            config[CONF_MOSI_PIN],
        )
    )
    cg.add(var.set_cold_mireds(config[CONF_COLD_WHITE]))
    cg.add(var.set_warm_mireds(config[CONF_WARM_WHITE]))

    # RF24 talks over SPI but doesn't declare it; ESPHome won't pull it in, so add both.
    cg.add_library("SPI", None)
    cg.add_library("nrf24/RF24", None)
