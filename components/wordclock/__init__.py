import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID

DEPENDENCIES = ["time", "light", "wifi"]
AUTO_LOAD = ["light", "switch", "select", "number", "button"]

wordclock_ns = cg.esphome_ns.namespace("wordclock")
WordClock = wordclock_ns.class_("WordClock", cg.Component)

LightType = wordclock_ns.enum("LightType")

LIGHT_TYPES = {
    "hours": LightType.LIGHT_HOURS,
    "minutes": LightType.LIGHT_MINUTES,
    "seconds": LightType.LIGHT_SECONDS,
    "background": LightType.LIGHT_BACKGROUND,
    "words": LightType.LIGHT_WORDS,
}

CONF_WORDCLOCK_ID = "wordclock_id"
CONF_NUM_LEDS = "num_leds"
CONF_TIME_ID = "time_id"
CONF_STRIP_ID = "strip_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WordClock),
        cv.Required(CONF_STRIP_ID): cv.use_id(light.AddressableLightState),
        cv.Optional(CONF_NUM_LEDS, default=256): cv.int_,
        cv.Required(CONF_TIME_ID): cv.use_id(cg.PollingComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
    strip = await cg.get_variable(config[CONF_STRIP_ID])
    cg.add(var.set_strip(strip))
    time_comp = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_comp))
