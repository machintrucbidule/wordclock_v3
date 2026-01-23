import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

from .. import wordclock_ns, WordClock, CONF_WORDCLOCK_ID, LIGHT_TYPES

WordClockLight = wordclock_ns.class_("WordClockLight", light.LightOutput, cg.Component)

CONF_LIGHT_TYPE = "light_type"

CONFIG_SCHEMA = (
    light.RGB_LIGHT_SCHEMA.extend({
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(WordClockLight),
        cv.Required(CONF_WORDCLOCK_ID): cv.use_id(WordClock),
        cv.Required(CONF_LIGHT_TYPE): cv.enum(LIGHT_TYPES),
    }).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_WORDCLOCK_ID])
    cg.add(var.set_wordclock(parent))
    cg.add(var.set_light_type(config[CONF_LIGHT_TYPE]))
    cg.add(parent.register_light(var, config[CONF_LIGHT_TYPE]))
