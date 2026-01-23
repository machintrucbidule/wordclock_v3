import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch

from .. import wordclock_ns, WordClock, CONF_WORDCLOCK_ID

WordClockSwitch = wordclock_ns.class_("WordClockSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = (
    switch.switch_schema(WordClockSwitch)
    .extend({
        cv.Required(CONF_WORDCLOCK_ID): cv.use_id(WordClock),
    })
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_WORDCLOCK_ID])
    cg.add(var.set_wordclock(parent))
    cg.add(parent.register_switch(var))
