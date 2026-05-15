import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, text_sensor, climate, switch, select
from esphome.const import (
    CONF_ID,
    UNIT_CELSIUS,
    ICON_THERMOMETER,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor", "climate", "switch", "select"]
CODEOWNERS = ["@kyleteal"]

pioneer_minisplit_ns = cg.esphome_ns.namespace("pioneer_minisplit")
PioneerMinisplit = pioneer_minisplit_ns.class_("PioneerMinisplit", climate.Climate, cg.Component, uart.UARTDevice)

# Switch classes for controllable features
PioneerSwitch = pioneer_minisplit_ns.class_("PioneerSwitch", switch.Switch, cg.Component)

# Select classes for swing positions
PioneerSelect = pioneer_minisplit_ns.class_("PioneerSelect", select.Select, cg.Component)

# Config keys
CONF_SET_TEMP = "set_temperature"
CONF_CURRENT_TEMP = "current_temperature"
CONF_PACKETS_TX = "packets_tx"
CONF_PACKETS_RX = "packets_rx"
CONF_POWER = "power"
CONF_MODE = "mode"
CONF_FAN = "fan"
CONF_SWING = "swing"
CONF_ECO = "eco"
CONF_TURBO = "turbo"
CONF_DISPLAY = "display"
CONF_SLEEP = "sleep"
CONF_HEALTH = "health"
CONF_TIMER = "timer"
CONF_TIMER_TIME = "timer_time"
CONF_HEATER_8C = "heater_8c"
CONF_MUTE = "mute"
CONF_BEEP = "beep"
CONF_SWING_V = "swing_v"
CONF_SWING_H = "swing_h"
CONF_BYTE9 = "byte9_raw"
CONF_BYTE10 = "byte10_raw"
CONF_BYTE18 = "byte18_raw"
CONF_BYTE19 = "byte19_raw"
CONF_BYTE30 = "byte30_raw"
CONF_BYTE32 = "byte32_raw"
CONF_BYTE33 = "byte33_raw"
CONF_BYTE34 = "byte34_raw"
CONF_BYTE35 = "byte35_raw"
CONF_BYTE36 = "byte36_raw"
CONF_BYTE37 = "byte37_raw"
CONF_BYTE50 = "byte50_raw"
CONF_BYTE51 = "byte51_raw"
CONF_BYTE30_HISTORY = "byte30_history"
CONF_BYTE34_HISTORY = "byte34_history"
CONF_BYTE35_HISTORY = "byte35_history"
CONF_BYTE36_HISTORY = "byte36_history"
CONF_BYTE37_HISTORY = "byte37_history"
CONF_LAST_TX = "last_tx"
CONF_LAST_RX = "last_rx"
CONF_COIL_TEMP = "coil_temperature"
CONF_DEBUG_BYTE30_F_DIV4 = "debug_byte30_f_div4"
CONF_DEBUG_BYTE34_F_DIV4 = "debug_byte34_f_div4"
CONF_DEBUG_BYTE35_F_DIV4 = "debug_byte35_f_div4"
CONF_DEBUG_BYTE36_F_DIV4 = "debug_byte36_f_div4"
CONF_DEBUG_BYTE37_F_DIV4 = "debug_byte37_f_div4"
CONF_DEBUG_PAIR_34_35_F_374 = "debug_pair_34_35_f_374"
CONF_DEBUG_PAIR_35_36_F_374 = "debug_pair_35_36_f_374"
CONF_DEBUG_PAIR_36_37_F_374 = "debug_pair_36_37_f_374"
CONF_DEBUG_BYTE30_STATE = "debug_byte30_state"
CONF_DEBUG_BYTE34_STATE = "debug_byte34_state"
CONF_DEBUG_BYTE35_STATE = "debug_byte35_state"
CONF_DEBUG_BYTE36_STATE = "debug_byte36_state"
CONF_DEBUG_BYTE37_STATE = "debug_byte37_state"
CONF_DEBUG_PAIR_34_35_STATE = "debug_pair_34_35_state"
CONF_DEBUG_PAIR_35_36_STATE = "debug_pair_35_36_state"
CONF_DEBUG_PAIR_36_37_STATE = "debug_pair_36_37_state"

# Outdoor/compressor sensors
CONF_OUTDOOR_TEMP = "outdoor_temp"  # Byte 35: value - 20 = °C
CONF_CONDENSER_COIL_TEMP = "condenser_coil_temp"  # Byte 36
CONF_DISCHARGE_TEMP = "discharge_temp"  # Byte 37: compressor discharge
CONF_COMPRESSOR_FREQ = "compressor_freq"  # Byte 38
CONF_INDOOR_FAN_RPM = "indoor_fan_rpm"  # Byte 34
CONF_OUTDOOR_FAN_SPEED = "outdoor_fan_speed"  # Byte 39
CONF_CURRENT_AMPS = "current_amps"  # Byte 46
CONF_HEAT_MODE_ACTIVE = "heat_mode_active"  # Byte 40 bit 6
CONF_OUTDOOR_RUNNING = "outdoor_running"  # Byte 40 bits 0-3 (0x00=Idle, 0x0A=Running)

# Switch config keys (only for features not covered by climate presets/modes)
CONF_DISPLAY_SWITCH = "display_switch"
CONF_BEEP_SWITCH = "beep_switch"
CONF_HEALTH_SWITCH = "health_switch"
CONF_HEATER_8C_SWITCH = "heater_8c_switch"

# Select config keys
CONF_SWING_V_SELECT = "swing_v_select"
CONF_SWING_H_SELECT = "swing_h_select"

# Aliases for easier config
CONF_INDOOR_TEMP = "indoor_temp"
CONF_COIL_TEMP_ALIAS = "coil_temp"
CONF_CURRENT = "current"
CONF_SWING_VERTICAL = "swing_vertical"
CONF_SWING_HORIZONTAL = "swing_horizontal"

CONFIG_SCHEMA = climate._CLIMATE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(PioneerMinisplit),
    cv.Optional(CONF_SET_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CURRENT_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_PACKETS_TX): sensor.sensor_schema(
        icon="mdi:arrow-right-bold",
        accuracy_decimals=0,
    ),
    cv.Optional(CONF_PACKETS_RX): sensor.sensor_schema(
        icon="mdi:arrow-left-bold",
        accuracy_decimals=0,
    ),
    cv.Optional(CONF_POWER): text_sensor.text_sensor_schema(icon="mdi:power"),
    cv.Optional(CONF_MODE): text_sensor.text_sensor_schema(icon="mdi:air-conditioner"),
    cv.Optional(CONF_FAN): text_sensor.text_sensor_schema(icon="mdi:fan"),
    cv.Optional(CONF_SWING): text_sensor.text_sensor_schema(icon="mdi:arrow-oscillating"),
    cv.Optional(CONF_ECO): text_sensor.text_sensor_schema(icon="mdi:leaf"),
    cv.Optional(CONF_TURBO): text_sensor.text_sensor_schema(icon="mdi:lightning-bolt"),
    cv.Optional(CONF_DISPLAY): text_sensor.text_sensor_schema(icon="mdi:monitor"),
    cv.Optional(CONF_SLEEP): text_sensor.text_sensor_schema(icon="mdi:sleep"),
    cv.Optional(CONF_HEALTH): text_sensor.text_sensor_schema(icon="mdi:hospital-box"),
    cv.Optional(CONF_TIMER): text_sensor.text_sensor_schema(icon="mdi:timer"),
    cv.Optional(CONF_TIMER_TIME): text_sensor.text_sensor_schema(icon="mdi:timer-outline"),
    cv.Optional(CONF_HEATER_8C): text_sensor.text_sensor_schema(icon="mdi:snowflake-thermometer"),
    cv.Optional(CONF_MUTE): text_sensor.text_sensor_schema(icon="mdi:volume-off"),
    cv.Optional(CONF_BEEP): text_sensor.text_sensor_schema(icon="mdi:volume-high"),
    cv.Optional(CONF_SWING_V): text_sensor.text_sensor_schema(icon="mdi:arrow-up-down"),
    cv.Optional(CONF_SWING_H): text_sensor.text_sensor_schema(icon="mdi:arrow-left-right"),
    cv.Optional(CONF_BYTE9): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE10): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE18): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE19): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE30): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE32): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE33): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE34): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE35): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE36): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE37): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE50): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE51): text_sensor.text_sensor_schema(icon="mdi:hexadecimal"),
    cv.Optional(CONF_BYTE30_HISTORY): text_sensor.text_sensor_schema(icon="mdi:history"),
    cv.Optional(CONF_BYTE34_HISTORY): text_sensor.text_sensor_schema(icon="mdi:history"),
    cv.Optional(CONF_BYTE35_HISTORY): text_sensor.text_sensor_schema(icon="mdi:history"),
    cv.Optional(CONF_BYTE36_HISTORY): text_sensor.text_sensor_schema(icon="mdi:history"),
    cv.Optional(CONF_BYTE37_HISTORY): text_sensor.text_sensor_schema(icon="mdi:history"),
    cv.Optional(CONF_LAST_TX): text_sensor.text_sensor_schema(icon="mdi:console"),
    cv.Optional(CONF_LAST_RX): text_sensor.text_sensor_schema(icon="mdi:console"),
    cv.Optional(CONF_COIL_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon="mdi:thermometer-water",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_BYTE30_F_DIV4): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_BYTE34_F_DIV4): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_BYTE35_F_DIV4): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_BYTE36_F_DIV4): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_BYTE37_F_DIV4): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_PAIR_34_35_F_374): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_PAIR_35_36_F_374): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_PAIR_36_37_F_374): sensor.sensor_schema(
        unit_of_measurement="°F", icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEBUG_BYTE30_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    cv.Optional(CONF_DEBUG_BYTE34_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    cv.Optional(CONF_DEBUG_BYTE35_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    cv.Optional(CONF_DEBUG_BYTE36_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    cv.Optional(CONF_DEBUG_BYTE37_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    cv.Optional(CONF_DEBUG_PAIR_34_35_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    cv.Optional(CONF_DEBUG_PAIR_35_36_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    cv.Optional(CONF_DEBUG_PAIR_36_37_STATE): sensor.sensor_schema(icon="mdi:counter", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
    # Outdoor/compressor sensors
    cv.Optional(CONF_OUTDOOR_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS, icon="mdi:thermometer", accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CONDENSER_COIL_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS, icon="mdi:thermometer-water", accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DISCHARGE_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS, icon="mdi:thermometer-high", accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_COMPRESSOR_FREQ): sensor.sensor_schema(
        unit_of_measurement="Hz", icon="mdi:sine-wave", accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_INDOOR_FAN_RPM): sensor.sensor_schema(
        unit_of_measurement="RPM", icon="mdi:fan", accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_OUTDOOR_FAN_SPEED): sensor.sensor_schema(
        icon="mdi:fan", accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CURRENT_AMPS): sensor.sensor_schema(
        unit_of_measurement="A", icon="mdi:current-ac", accuracy_decimals=1,
        device_class="current", state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_HEAT_MODE_ACTIVE): text_sensor.text_sensor_schema(icon="mdi:fire"),
    cv.Optional(CONF_OUTDOOR_RUNNING): text_sensor.text_sensor_schema(icon="mdi:fan"),
    # Control switches (features not covered by climate presets/modes)
    cv.Optional(CONF_DISPLAY_SWITCH): switch.switch_schema(PioneerSwitch, icon="mdi:monitor"),
    cv.Optional(CONF_BEEP_SWITCH): switch.switch_schema(PioneerSwitch, icon="mdi:volume-high"),
    cv.Optional(CONF_HEALTH_SWITCH): switch.switch_schema(PioneerSwitch, icon="mdi:hospital-box"),
    cv.Optional(CONF_HEATER_8C_SWITCH): switch.switch_schema(PioneerSwitch, icon="mdi:snowflake-thermometer"),
    # Swing position selects
    cv.Optional(CONF_SWING_V_SELECT): select.select_schema(PioneerSelect, icon="mdi:arrow-up-down"),
    cv.Optional(CONF_SWING_H_SELECT): select.select_schema(PioneerSelect, icon="mdi:arrow-left-right"),
    # Aliases
    cv.Optional(CONF_INDOOR_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS, icon=ICON_THERMOMETER, accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_COIL_TEMP_ALIAS): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS, icon="mdi:thermometer-water", accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CURRENT): sensor.sensor_schema(
        unit_of_measurement="A", icon="mdi:current-ac", accuracy_decimals=1,
        device_class="current", state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_SWING_VERTICAL): select.select_schema(PioneerSelect, icon="mdi:arrow-up-down"),
    cv.Optional(CONF_SWING_HORIZONTAL): select.select_schema(PioneerSelect, icon="mdi:arrow-left-right"),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)
    
    # Sensors
    if CONF_SET_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_SET_TEMP])
        cg.add(var.set_set_temp_sensor(sens))
    if CONF_CURRENT_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_TEMP])
        cg.add(var.set_current_temp_sensor(sens))
    if CONF_INDOOR_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_INDOOR_TEMP])
        cg.add(var.set_current_temp_sensor(sens))
    if CONF_PACKETS_TX in config:
        sens = await sensor.new_sensor(config[CONF_PACKETS_TX])
        cg.add(var.set_packets_tx_sensor(sens))
    if CONF_PACKETS_RX in config:
        sens = await sensor.new_sensor(config[CONF_PACKETS_RX])
        cg.add(var.set_packets_rx_sensor(sens))
    if CONF_COIL_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_COIL_TEMP])
        cg.add(var.set_coil_temp_sensor(sens))
    if CONF_COIL_TEMP_ALIAS in config:
        sens = await sensor.new_sensor(config[CONF_COIL_TEMP_ALIAS])
        cg.add(var.set_coil_temp_sensor(sens))
    
    # Outdoor/compressor sensors
    if CONF_OUTDOOR_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_OUTDOOR_TEMP])
        cg.add(var.set_outdoor_temp_sensor(sens))
    if CONF_CONDENSER_COIL_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_CONDENSER_COIL_TEMP])
        cg.add(var.set_condenser_coil_temp_sensor(sens))
    if CONF_DISCHARGE_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_DISCHARGE_TEMP])
        cg.add(var.set_discharge_temp_sensor(sens))
    if CONF_COMPRESSOR_FREQ in config:
        sens = await sensor.new_sensor(config[CONF_COMPRESSOR_FREQ])
        cg.add(var.set_compressor_freq_sensor(sens))
    if CONF_INDOOR_FAN_RPM in config:
        sens = await sensor.new_sensor(config[CONF_INDOOR_FAN_RPM])
        cg.add(var.set_indoor_fan_rpm_sensor(sens))
    if CONF_OUTDOOR_FAN_SPEED in config:
        sens = await sensor.new_sensor(config[CONF_OUTDOOR_FAN_SPEED])
        cg.add(var.set_outdoor_fan_speed_sensor(sens))
    if CONF_CURRENT_AMPS in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_AMPS])
        cg.add(var.set_current_amps_sensor(sens))
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_amps_sensor(sens))
    
    # Text sensors
    if CONF_POWER in config:
        sens = await text_sensor.new_text_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))
    if CONF_MODE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MODE])
        cg.add(var.set_mode_sensor(sens))
    if CONF_FAN in config:
        sens = await text_sensor.new_text_sensor(config[CONF_FAN])
        cg.add(var.set_fan_sensor(sens))
    if CONF_SWING in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SWING])
        cg.add(var.set_swing_sensor(sens))
    if CONF_ECO in config:
        sens = await text_sensor.new_text_sensor(config[CONF_ECO])
        cg.add(var.set_eco_sensor(sens))
    if CONF_TURBO in config:
        sens = await text_sensor.new_text_sensor(config[CONF_TURBO])
        cg.add(var.set_turbo_sensor(sens))
    if CONF_DISPLAY in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DISPLAY])
        cg.add(var.set_display_sensor(sens))
    if CONF_SLEEP in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SLEEP])
        cg.add(var.set_sleep_sensor(sens))
    if CONF_HEALTH in config:
        sens = await text_sensor.new_text_sensor(config[CONF_HEALTH])
        cg.add(var.set_health_sensor(sens))
    if CONF_TIMER in config:
        sens = await text_sensor.new_text_sensor(config[CONF_TIMER])
        cg.add(var.set_timer_sensor(sens))
    if CONF_TIMER_TIME in config:
        sens = await text_sensor.new_text_sensor(config[CONF_TIMER_TIME])
        cg.add(var.set_timer_time_sensor(sens))
    if CONF_HEATER_8C in config:
        sens = await text_sensor.new_text_sensor(config[CONF_HEATER_8C])
        cg.add(var.set_heater_8c_sensor(sens))
    if CONF_MUTE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MUTE])
        cg.add(var.set_mute_sensor(sens))
    if CONF_BEEP in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BEEP])
        cg.add(var.set_beep_sensor(sens))
    if CONF_SWING_V in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SWING_V])
        cg.add(var.set_swing_v_sensor(sens))
    if CONF_SWING_H in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SWING_H])
        cg.add(var.set_swing_h_sensor(sens))
    if CONF_HEAT_MODE_ACTIVE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_HEAT_MODE_ACTIVE])
        cg.add(var.set_heat_mode_active_sensor(sens))
    if CONF_OUTDOOR_RUNNING in config:
        sens = await text_sensor.new_text_sensor(config[CONF_OUTDOOR_RUNNING])
        cg.add(var.set_outdoor_running_sensor(sens))
    if CONF_BYTE9 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE9])
        cg.add(var.set_byte9_sensor(sens))
    if CONF_BYTE10 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE10])
        cg.add(var.set_byte10_sensor(sens))
    if CONF_BYTE18 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE18])
        cg.add(var.set_byte18_sensor(sens))
    if CONF_BYTE19 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE19])
        cg.add(var.set_byte19_sensor(sens))
    if CONF_BYTE30 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE30])
        cg.add(var.set_byte30_sensor(sens))
    if CONF_BYTE32 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE32])
        cg.add(var.set_byte32_sensor(sens))
    if CONF_BYTE33 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE33])
        cg.add(var.set_byte33_sensor(sens))
    if CONF_BYTE34 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE34])
        cg.add(var.set_byte34_sensor(sens))
    if CONF_BYTE35 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE35])
        cg.add(var.set_byte35_sensor(sens))
    if CONF_BYTE36 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE36])
        cg.add(var.set_byte36_sensor(sens))
    if CONF_BYTE37 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE37])
        cg.add(var.set_byte37_sensor(sens))
    if CONF_BYTE50 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE50])
        cg.add(var.set_byte50_sensor(sens))
    if CONF_BYTE51 in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE51])
        cg.add(var.set_byte51_sensor(sens))
    if CONF_BYTE30_HISTORY in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE30_HISTORY])
        cg.add(var.set_byte30_history_sensor(sens))
    if CONF_BYTE34_HISTORY in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE34_HISTORY])
        cg.add(var.set_byte34_history_sensor(sens))
    if CONF_BYTE35_HISTORY in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE35_HISTORY])
        cg.add(var.set_byte35_history_sensor(sens))
    if CONF_BYTE36_HISTORY in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE36_HISTORY])
        cg.add(var.set_byte36_history_sensor(sens))
    if CONF_BYTE37_HISTORY in config:
        sens = await text_sensor.new_text_sensor(config[CONF_BYTE37_HISTORY])
        cg.add(var.set_byte37_history_sensor(sens))
    if CONF_LAST_TX in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_TX])
        cg.add(var.set_last_tx_sensor(sens))
    if CONF_LAST_RX in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_RX])
        cg.add(var.set_last_rx_sensor(sens))
    
    # Debug sensors
    if CONF_DEBUG_BYTE30_F_DIV4 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE30_F_DIV4])
        cg.add(var.set_debug_byte30_f_div4_sensor(sens))
    if CONF_DEBUG_BYTE34_F_DIV4 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE34_F_DIV4])
        cg.add(var.set_debug_byte34_f_div4_sensor(sens))
    if CONF_DEBUG_BYTE35_F_DIV4 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE35_F_DIV4])
        cg.add(var.set_debug_byte35_f_div4_sensor(sens))
    if CONF_DEBUG_BYTE36_F_DIV4 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE36_F_DIV4])
        cg.add(var.set_debug_byte36_f_div4_sensor(sens))
    if CONF_DEBUG_BYTE37_F_DIV4 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE37_F_DIV4])
        cg.add(var.set_debug_byte37_f_div4_sensor(sens))
    if CONF_DEBUG_PAIR_34_35_F_374 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_PAIR_34_35_F_374])
        cg.add(var.set_debug_pair_34_35_f_374_sensor(sens))
    if CONF_DEBUG_PAIR_35_36_F_374 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_PAIR_35_36_F_374])
        cg.add(var.set_debug_pair_35_36_f_374_sensor(sens))
    if CONF_DEBUG_PAIR_36_37_F_374 in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_PAIR_36_37_F_374])
        cg.add(var.set_debug_pair_36_37_f_374_sensor(sens))
    if CONF_DEBUG_BYTE30_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE30_STATE])
        cg.add(var.set_debug_byte30_state_sensor(sens))
    if CONF_DEBUG_BYTE34_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE34_STATE])
        cg.add(var.set_debug_byte34_state_sensor(sens))
    if CONF_DEBUG_BYTE35_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE35_STATE])
        cg.add(var.set_debug_byte35_state_sensor(sens))
    if CONF_DEBUG_BYTE36_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE36_STATE])
        cg.add(var.set_debug_byte36_state_sensor(sens))
    if CONF_DEBUG_BYTE37_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_BYTE37_STATE])
        cg.add(var.set_debug_byte37_state_sensor(sens))
    if CONF_DEBUG_PAIR_34_35_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_PAIR_34_35_STATE])
        cg.add(var.set_debug_pair_34_35_state_sensor(sens))
    if CONF_DEBUG_PAIR_35_36_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_PAIR_35_36_STATE])
        cg.add(var.set_debug_pair_35_36_state_sensor(sens))
    if CONF_DEBUG_PAIR_36_37_STATE in config:
        sens = await sensor.new_sensor(config[CONF_DEBUG_PAIR_36_37_STATE])
        cg.add(var.set_debug_pair_36_37_state_sensor(sens))
    
    # Control switches
    if CONF_DISPLAY_SWITCH in config:
        sw = await switch.new_switch(config[CONF_DISPLAY_SWITCH])
        cg.add(var.set_display_switch(sw))
        cg.add(sw.set_parent(var))
        cg.add(sw.set_type(0))  # DISPLAY
    if CONF_BEEP_SWITCH in config:
        sw = await switch.new_switch(config[CONF_BEEP_SWITCH])
        cg.add(var.set_beep_switch(sw))
        cg.add(sw.set_parent(var))
        cg.add(sw.set_type(1))  # BEEP
    if CONF_HEALTH_SWITCH in config:
        sw = await switch.new_switch(config[CONF_HEALTH_SWITCH])
        cg.add(var.set_health_switch(sw))
        cg.add(sw.set_parent(var))
        cg.add(sw.set_type(2))  # HEALTH
    if CONF_HEATER_8C_SWITCH in config:
        sw = await switch.new_switch(config[CONF_HEATER_8C_SWITCH])
        cg.add(var.set_heater_8c_switch(sw))
        cg.add(sw.set_parent(var))
        cg.add(sw.set_type(3))  # HEATER_8C
    
    # Swing selects
    if CONF_SWING_V_SELECT in config:
        sel = await select.new_select(config[CONF_SWING_V_SELECT], options=[
            "Off", "Swing - Auto", "Swing - Upper", "Swing - Lower",
            "Fixed Top", "Fixed Upper", "Fixed Middle", "Fixed Lower", "Fixed Bottom"
        ])
        cg.add(var.set_swing_v_select(sel))
        cg.add(sel.set_parent(var))
        cg.add(sel.set_type(0))  # SWING_V
    if CONF_SWING_H_SELECT in config:
        sel = await select.new_select(config[CONF_SWING_H_SELECT], options=[
            "Off", "Swing - Auto", "Swing - Left", "Swing - Center", "Swing - Right",
            "Fixed Far Left", "Fixed Left", "Fixed Center", "Fixed Right", "Fixed Far Right"
        ])
        cg.add(var.set_swing_h_select(sel))
        cg.add(sel.set_parent(var))
        cg.add(sel.set_type(1))  # SWING_H
    # Swing aliases
    if CONF_SWING_VERTICAL in config:
        sel = await select.new_select(config[CONF_SWING_VERTICAL], options=[
            "Off", "Swing - Auto", "Swing - Upper", "Swing - Lower",
            "Fixed Top", "Fixed Upper", "Fixed Middle", "Fixed Lower", "Fixed Bottom"
        ])
        cg.add(var.set_swing_v_select(sel))
        cg.add(sel.set_parent(var))
        cg.add(sel.set_type(0))  # SWING_V
    if CONF_SWING_HORIZONTAL in config:
        sel = await select.new_select(config[CONF_SWING_HORIZONTAL], options=[
            "Off", "Swing - Auto", "Swing - Left", "Swing - Center", "Swing - Right",
            "Fixed Far Left", "Fixed Left", "Fixed Center", "Fixed Right", "Fixed Far Right"
        ])
        cg.add(var.set_swing_h_select(sel))
        cg.add(sel.set_parent(var))
        cg.add(sel.set_type(1))  # SWING_H
