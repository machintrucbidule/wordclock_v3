import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button

from .. import wordclock_ns, WordClock, CONF_WORDCLOCK_ID

WordClockFactoryResetButton = wordclock_ns.class_("WordClockFactoryResetButton", button.Button, cg.Component)

CONFIG_SCHEMA = (
    button.button_schema(WordClockFactoryResetButton)
    .extend({
        cv.Required(CONF_WORDCLOCK_ID): cv.use_id(WordClock),
    })
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_WORDCLOCK_ID])
    cg.add(var.set_wordclock(parent))
