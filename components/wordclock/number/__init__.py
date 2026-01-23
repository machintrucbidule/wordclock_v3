import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID, CONF_MIN_VALUE, CONF_MAX_VALUE, CONF_STEP

from .. import wordclock_ns, WordClock, CONF_WORDCLOCK_ID

WordClockNumber = wordclock_ns.class_("WordClockNumber", number.Number, cg.Component)

CONF_NUMBER_TYPE = "number_type"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(WordClockNumber),
    cv.Required(CONF_WORDCLOCK_ID): cv.use_id(WordClock),
    cv.Required(CONF_NUMBER_TYPE): cv.one_of(
        "words_fade_in", "words_fade_out", "seconds_fade_out", "typing_delay",
        "rainbow_spread", "words_effect_brightness", "effect_speed", "seconds_effect_brightness",
        lower=True
    ),
    cv.Optional(CONF_MIN_VALUE, default=0.0): cv.float_,
    cv.Optional(CONF_MAX_VALUE, default=100.0): cv.float_,
    cv.Optional(CONF_STEP, default=1.0): cv.float_,
}).extend(number._NUMBER_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await number.register_number(var, config, 
        min_value=config[CONF_MIN_VALUE], 
        max_value=config[CONF_MAX_VALUE], 
        step=config[CONF_STEP])
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_WORDCLOCK_ID])
    cg.add(var.set_wordclock(parent))
    type_map = {
        "words_fade_in": 0, "words_fade_out": 1, "seconds_fade_out": 2, "typing_delay": 3,
        "rainbow_spread": 4, "words_effect_brightness": 5, "effect_speed": 6, "seconds_effect_brightness": 7,
    }
    cg.add(var.set_number_type(type_map[config[CONF_NUMBER_TYPE]]))
