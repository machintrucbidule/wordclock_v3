import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

from .. import wordclock_ns, WordClock, CONF_WORDCLOCK_ID

WordClockSecondsSelect = wordclock_ns.class_("WordClockSecondsSelect", select.Select, cg.Component)
WordClockEffectSelect = wordclock_ns.class_("WordClockEffectSelect", select.Select, cg.Component)
WordClockLanguageSelect = wordclock_ns.class_("WordClockLanguageSelect", select.Select, cg.Component)

LightType = wordclock_ns.enum("LightType")

EFFECT_LIGHT_TYPES = {
    "words": LightType.LIGHT_WORDS,
    "seconds": LightType.LIGHT_SECONDS,
}

CONF_LIGHT_TYPE = "light_type"
CONF_SELECT_TYPE = "select_type"

# Select types
SELECT_TYPE_SECONDS = "seconds_mode"
SELECT_TYPE_EFFECT = "effect"
SELECT_TYPE_LANGUAGE = "language"

BASE_SCHEMA = cv.Schema({
    cv.Required(CONF_WORDCLOCK_ID): cv.use_id(WordClock),
    cv.Optional(CONF_LIGHT_TYPE): cv.enum(EFFECT_LIGHT_TYPES),
    cv.Optional(CONF_SELECT_TYPE): cv.one_of(SELECT_TYPE_SECONDS, SELECT_TYPE_EFFECT, SELECT_TYPE_LANGUAGE),
}).extend(select._SELECT_SCHEMA).extend(cv.COMPONENT_SCHEMA)

def CONFIG_SCHEMA(config):
    config = BASE_SCHEMA(config)
    
    # Determine select type based on configuration
    if config.get(CONF_SELECT_TYPE) == SELECT_TYPE_LANGUAGE:
        config[CONF_ID] = cv.declare_id(WordClockLanguageSelect)(config.get(CONF_ID))
    elif CONF_LIGHT_TYPE in config:
        config[CONF_ID] = cv.declare_id(WordClockEffectSelect)(config.get(CONF_ID))
    else:
        config[CONF_ID] = cv.declare_id(WordClockSecondsSelect)(config.get(CONF_ID))
    return config

async def to_code(config):
    select_type = config.get(CONF_SELECT_TYPE)
    
    if select_type == SELECT_TYPE_LANGUAGE:
        # Language selector
        var = cg.new_Pvariable(config[CONF_ID])
        # Options: French first (default), then English UK
        await select.register_select(var, config, options=["Francais", "English UK"])
        await cg.register_component(var, config)
        parent = await cg.get_variable(config[CONF_WORDCLOCK_ID])
        cg.add(var.set_wordclock(parent))
        cg.add(parent.register_language_select(var))
    elif CONF_LIGHT_TYPE in config:
        # Effect selector
        var = cg.new_Pvariable(config[CONF_ID])
        await select.register_select(var, config, options=["None", "Rainbow", "Pulse", "Breathe", "Color cycle"])
        await cg.register_component(var, config)
        parent = await cg.get_variable(config[CONF_WORDCLOCK_ID])
        cg.add(var.set_wordclock(parent))
        cg.add(var.set_light_type(config[CONF_LIGHT_TYPE]))
        cg.add(parent.register_effect_select(var, config[CONF_LIGHT_TYPE]))
    else:
        # Seconds mode selector
        var = cg.new_Pvariable(config[CONF_ID])
        await select.register_select(var, config, options=["Current second", "Seconds passed", "Inverted"])
        await cg.register_component(var, config)
        parent = await cg.get_variable(config[CONF_WORDCLOCK_ID])
        cg.add(var.set_wordclock(parent))
        cg.add(parent.register_seconds_select(var))
