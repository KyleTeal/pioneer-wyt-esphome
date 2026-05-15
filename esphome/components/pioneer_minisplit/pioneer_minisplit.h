#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace pioneer_minisplit {

// Forward declaration
class PioneerMinisplit;

// Struct for persisting swing state to flash
struct SwingState {
  uint8_t swing_v;
  bool swing_v_active;
  uint8_t swing_h;
};

// Switch types (only for features not covered by climate presets/modes)
enum SwitchType : uint8_t {
  SWITCH_DISPLAY = 0,
  SWITCH_BEEP = 1,
  SWITCH_HEALTH = 2,
  SWITCH_HEATER_8C = 3,
};

// Select types
enum SelectType : uint8_t {
  SELECT_SWING_V = 0,
  SELECT_SWING_H = 1,
};

// Generic switch class
class PioneerSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PioneerMinisplit *parent) { this->parent_ = parent; }
  void set_type(uint8_t type) { this->type_ = type; }
  void write_state(bool state) override;
 protected:
  PioneerMinisplit *parent_{nullptr};
  uint8_t type_{0};
};

// Generic select class
class PioneerSelect : public select::Select, public Component {
 public:
  void set_parent(PioneerMinisplit *parent) { this->parent_ = parent; }
  void set_type(uint8_t type) { this->type_ = type; }
  void control(const std::string &value) override;
 protected:
  PioneerMinisplit *parent_{nullptr};
  uint8_t type_{0};
};

class PioneerMinisplit : public climate::Climate, public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  // Climate control
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;

  // Called by switches
  void set_feature(SwitchType type, bool state);
  // Called by selects
  void set_swing_position(SelectType type, const std::string &value);
  void queue_command() { this->command_pending_ = true; }

  // Switch setters (only for features not covered by climate presets/modes)
  void set_display_switch(PioneerSwitch *sw) { this->display_switch_ = sw; }
  void set_beep_switch(PioneerSwitch *sw) { this->beep_switch_ = sw; }
  void set_health_switch(PioneerSwitch *sw) { this->health_switch_ = sw; }
  void set_heater_8c_switch(PioneerSwitch *sw) { this->heater_8c_switch_ = sw; }

  // Select setters
  void set_swing_v_select(PioneerSelect *sel) { this->swing_v_select_ = sel; }
  void set_swing_h_select(PioneerSelect *sel) { this->swing_h_select_ = sel; }

  // Sensor setters
  void set_set_temp_sensor(sensor::Sensor *s) { set_temp_sensor_ = s; }
  void set_current_temp_sensor(sensor::Sensor *s) { current_temp_sensor_ = s; }
  void set_packets_tx_sensor(sensor::Sensor *s) { packets_tx_sensor_ = s; }
  void set_packets_rx_sensor(sensor::Sensor *s) { packets_rx_sensor_ = s; }
  void set_coil_temp_sensor(sensor::Sensor *s) { coil_temp_sensor_ = s; }

  void set_power_sensor(text_sensor::TextSensor *s) { power_sensor_ = s; }
  void set_mode_sensor(text_sensor::TextSensor *s) { mode_sensor_ = s; }
  void set_fan_sensor(text_sensor::TextSensor *s) { fan_sensor_ = s; }
  void set_swing_sensor(text_sensor::TextSensor *s) { swing_sensor_ = s; }
  void set_eco_sensor(text_sensor::TextSensor *s) { eco_sensor_ = s; }
  void set_turbo_sensor(text_sensor::TextSensor *s) { turbo_sensor_ = s; }
  void set_display_sensor(text_sensor::TextSensor *s) { display_sensor_ = s; }
  void set_sleep_sensor(text_sensor::TextSensor *s) { sleep_sensor_ = s; }
  void set_health_sensor(text_sensor::TextSensor *s) { health_sensor_ = s; }
  void set_timer_sensor(text_sensor::TextSensor *s) { timer_sensor_ = s; }
  void set_timer_time_sensor(text_sensor::TextSensor *s) { timer_time_sensor_ = s; }
  void set_heater_8c_sensor(text_sensor::TextSensor *s) { heater_8c_sensor_ = s; }
  void set_mute_sensor(text_sensor::TextSensor *s) { mute_sensor_ = s; }
  void set_beep_sensor(text_sensor::TextSensor *s) { beep_sensor_ = s; }
  void set_swing_v_sensor(text_sensor::TextSensor *s) { swing_v_sensor_ = s; }
  void set_swing_h_sensor(text_sensor::TextSensor *s) { swing_h_sensor_ = s; }
  void set_byte9_sensor(text_sensor::TextSensor *s) { byte9_sensor_ = s; }
  void set_byte10_sensor(text_sensor::TextSensor *s) { byte10_sensor_ = s; }
  void set_byte18_sensor(text_sensor::TextSensor *s) { byte18_sensor_ = s; }
  void set_byte19_sensor(text_sensor::TextSensor *s) { byte19_sensor_ = s; }
  void set_byte30_sensor(text_sensor::TextSensor *s) { byte30_sensor_ = s; }
  void set_byte32_sensor(text_sensor::TextSensor *s) { byte32_sensor_ = s; }
  void set_byte33_sensor(text_sensor::TextSensor *s) { byte33_sensor_ = s; }
  void set_byte34_sensor(text_sensor::TextSensor *s) { byte34_sensor_ = s; }
  void set_byte35_sensor(text_sensor::TextSensor *s) { byte35_sensor_ = s; }
  void set_byte36_sensor(text_sensor::TextSensor *s) { byte36_sensor_ = s; }
  void set_byte37_sensor(text_sensor::TextSensor *s) { byte37_sensor_ = s; }
  void set_byte50_sensor(text_sensor::TextSensor *s) { byte50_sensor_ = s; }
  void set_byte51_sensor(text_sensor::TextSensor *s) { byte51_sensor_ = s; }
  void set_byte30_history_sensor(text_sensor::TextSensor *s) { byte30_history_sensor_ = s; }
  void set_byte34_history_sensor(text_sensor::TextSensor *s) { byte34_history_sensor_ = s; }
  void set_byte35_history_sensor(text_sensor::TextSensor *s) { byte35_history_sensor_ = s; }
  void set_byte36_history_sensor(text_sensor::TextSensor *s) { byte36_history_sensor_ = s; }
  void set_byte37_history_sensor(text_sensor::TextSensor *s) { byte37_history_sensor_ = s; }
  void set_last_tx_sensor(text_sensor::TextSensor *s) { last_tx_sensor_ = s; }
  void set_last_rx_sensor(text_sensor::TextSensor *s) { last_rx_sensor_ = s; }

  void set_debug_byte30_f_div4_sensor(sensor::Sensor *s) { debug_byte30_f_div4_ = s; }
  void set_debug_byte34_f_div4_sensor(sensor::Sensor *s) { debug_byte34_f_div4_ = s; }
  void set_debug_byte35_f_div4_sensor(sensor::Sensor *s) { debug_byte35_f_div4_ = s; }
  void set_debug_byte36_f_div4_sensor(sensor::Sensor *s) { debug_byte36_f_div4_ = s; }
  void set_debug_byte37_f_div4_sensor(sensor::Sensor *s) { debug_byte37_f_div4_ = s; }
  void set_debug_pair_34_35_f_374_sensor(sensor::Sensor *s) { debug_pair_34_35_f_374_ = s; }
  void set_debug_pair_35_36_f_374_sensor(sensor::Sensor *s) { debug_pair_35_36_f_374_ = s; }
  void set_debug_pair_36_37_f_374_sensor(sensor::Sensor *s) { debug_pair_36_37_f_374_ = s; }
  void set_debug_byte30_state_sensor(sensor::Sensor *s) { debug_byte30_state_ = s; }
  void set_debug_byte34_state_sensor(sensor::Sensor *s) { debug_byte34_state_ = s; }
  void set_debug_byte35_state_sensor(sensor::Sensor *s) { debug_byte35_state_ = s; }
  void set_debug_byte36_state_sensor(sensor::Sensor *s) { debug_byte36_state_ = s; }
  void set_debug_byte37_state_sensor(sensor::Sensor *s) { debug_byte37_state_ = s; }
  void set_debug_pair_34_35_state_sensor(sensor::Sensor *s) { debug_pair_34_35_state_ = s; }
  void set_debug_pair_35_36_state_sensor(sensor::Sensor *s) { debug_pair_35_36_state_ = s; }
  void set_debug_pair_36_37_state_sensor(sensor::Sensor *s) { debug_pair_36_37_state_ = s; }

  // Outdoor/compressor sensors
  void set_outdoor_temp_sensor(sensor::Sensor *s) { outdoor_temp_sensor_ = s; }
  void set_condenser_coil_temp_sensor(sensor::Sensor *s) { condenser_coil_temp_sensor_ = s; }
  void set_discharge_temp_sensor(sensor::Sensor *s) { discharge_temp_sensor_ = s; }
  void set_compressor_freq_sensor(sensor::Sensor *s) { compressor_freq_sensor_ = s; }
  void set_indoor_fan_rpm_sensor(sensor::Sensor *s) { indoor_fan_rpm_sensor_ = s; }
  void set_outdoor_fan_speed_sensor(sensor::Sensor *s) { outdoor_fan_speed_sensor_ = s; }
  void set_current_amps_sensor(sensor::Sensor *s) { current_amps_sensor_ = s; }
  void set_heat_mode_active_sensor(text_sensor::TextSensor *s) { heat_mode_active_sensor_ = s; }
  void set_outdoor_running_sensor(text_sensor::TextSensor *s) { outdoor_running_sensor_ = s; }

 protected:
  void send_heartbeat_();
  void send_command_();
  void process_packet_(uint8_t *buf, size_t len);
  void decode_rx_packet_(uint8_t *buf, size_t len);
  uint8_t calculate_checksum_(const uint8_t *data, size_t len);
  std::string hex_string_(uint8_t *buf, size_t len);
  
  const char* mode_str_(uint8_t mode);
  const char* fan_full_str_(uint8_t fan, bool turbo, bool mute);
  const char* sleep_str_(uint8_t sleep);
  const char* swing_v_rx_str_(uint8_t byte50, bool swing_active);
  const char* swing_h_rx_str_(uint8_t byte51, bool swing_active);
  float c_to_f_(float c) { return (c * 9.0f / 5.0f) + 32.0f; }
  
  void update_history_(uint8_t *history, uint8_t value);
  std::string format_history_(uint8_t *history);

  // Switches (only for features not covered by climate presets/modes)
  PioneerSwitch *display_switch_{nullptr};
  PioneerSwitch *beep_switch_{nullptr};
  PioneerSwitch *health_switch_{nullptr};
  PioneerSwitch *heater_8c_switch_{nullptr};

  // Selects
  PioneerSelect *swing_v_select_{nullptr};
  PioneerSelect *swing_h_select_{nullptr};

  // RX buffer
  uint8_t rx_buffer_[64];
  size_t rx_index_ = 0;
  uint32_t last_rx_time_ = 0;
  
  // Timing
  uint32_t last_heartbeat_ = 0;
  uint32_t last_command_ = 0;
  bool command_pending_ = false;
  uint32_t tx_count_ = 0;
  uint32_t rx_count_ = 0;
  
  static const uint32_t HEARTBEAT_INTERVAL = 3000;
  static const uint32_t RX_TIMEOUT = 100;
  static const uint32_t COMMAND_COOLDOWN = 500;

  // History for unknown bytes
  uint8_t byte30_history_[5] = {0};
  uint8_t byte34_history_[5] = {0};
  uint8_t byte35_history_[5] = {0};
  uint8_t byte36_history_[5] = {0};
  uint8_t byte37_history_[5] = {0};

  // TX pending state
  bool pending_power_ = false;
  uint8_t pending_mode_ = 0x03;
  uint8_t pending_fan_ = 0x38;
  uint8_t pending_temp_ = 24;
  uint8_t pending_swing_v_ = 0x08;
  uint8_t pending_swing_h_ = 0x80;
  bool pending_swing_v_active_ = true;
  ESPPreferenceObject pref_;
  bool pending_display_ = true;
  bool pending_beep_ = false;
  bool pending_eco_ = false;
  bool pending_turbo_ = false;
  bool pending_health_ = false;
  bool pending_mute_ = false;
  bool pending_8c_heater_ = false;
  uint8_t pending_sleep_ = 0;
  
  // Track if we've synced state from HVAC at least once
  bool state_synced_ = false;

  // Sensors
  sensor::Sensor *set_temp_sensor_{nullptr};
  sensor::Sensor *current_temp_sensor_{nullptr};
  sensor::Sensor *packets_tx_sensor_{nullptr};
  sensor::Sensor *packets_rx_sensor_{nullptr};
  sensor::Sensor *coil_temp_sensor_{nullptr};

  text_sensor::TextSensor *power_sensor_{nullptr};
  text_sensor::TextSensor *mode_sensor_{nullptr};
  text_sensor::TextSensor *fan_sensor_{nullptr};
  text_sensor::TextSensor *swing_sensor_{nullptr};
  text_sensor::TextSensor *eco_sensor_{nullptr};
  text_sensor::TextSensor *turbo_sensor_{nullptr};
  text_sensor::TextSensor *display_sensor_{nullptr};
  text_sensor::TextSensor *sleep_sensor_{nullptr};
  text_sensor::TextSensor *health_sensor_{nullptr};
  text_sensor::TextSensor *timer_sensor_{nullptr};
  text_sensor::TextSensor *timer_time_sensor_{nullptr};
  text_sensor::TextSensor *heater_8c_sensor_{nullptr};
  text_sensor::TextSensor *mute_sensor_{nullptr};
  text_sensor::TextSensor *beep_sensor_{nullptr};
  text_sensor::TextSensor *swing_v_sensor_{nullptr};
  text_sensor::TextSensor *swing_h_sensor_{nullptr};
  text_sensor::TextSensor *byte9_sensor_{nullptr};
  text_sensor::TextSensor *byte10_sensor_{nullptr};
  text_sensor::TextSensor *byte18_sensor_{nullptr};
  text_sensor::TextSensor *byte19_sensor_{nullptr};
  text_sensor::TextSensor *byte30_sensor_{nullptr};
  text_sensor::TextSensor *byte32_sensor_{nullptr};
  text_sensor::TextSensor *byte33_sensor_{nullptr};
  text_sensor::TextSensor *byte34_sensor_{nullptr};
  text_sensor::TextSensor *byte35_sensor_{nullptr};
  text_sensor::TextSensor *byte36_sensor_{nullptr};
  text_sensor::TextSensor *byte37_sensor_{nullptr};
  text_sensor::TextSensor *byte50_sensor_{nullptr};
  text_sensor::TextSensor *byte51_sensor_{nullptr};
  text_sensor::TextSensor *byte30_history_sensor_{nullptr};
  text_sensor::TextSensor *byte34_history_sensor_{nullptr};
  text_sensor::TextSensor *byte35_history_sensor_{nullptr};
  text_sensor::TextSensor *byte36_history_sensor_{nullptr};
  text_sensor::TextSensor *byte37_history_sensor_{nullptr};
  text_sensor::TextSensor *last_tx_sensor_{nullptr};
  text_sensor::TextSensor *last_rx_sensor_{nullptr};

  sensor::Sensor *debug_byte30_f_div4_{nullptr};
  sensor::Sensor *debug_byte34_f_div4_{nullptr};
  sensor::Sensor *debug_byte35_f_div4_{nullptr};
  sensor::Sensor *debug_byte36_f_div4_{nullptr};
  sensor::Sensor *debug_byte37_f_div4_{nullptr};
  sensor::Sensor *debug_pair_34_35_f_374_{nullptr};
  sensor::Sensor *debug_pair_35_36_f_374_{nullptr};
  sensor::Sensor *debug_pair_36_37_f_374_{nullptr};
  sensor::Sensor *debug_byte30_state_{nullptr};
  sensor::Sensor *debug_byte34_state_{nullptr};
  sensor::Sensor *debug_byte35_state_{nullptr};
  sensor::Sensor *debug_byte36_state_{nullptr};
  sensor::Sensor *debug_byte37_state_{nullptr};
  sensor::Sensor *debug_pair_34_35_state_{nullptr};
  sensor::Sensor *debug_pair_35_36_state_{nullptr};
  sensor::Sensor *debug_pair_36_37_state_{nullptr};

  // Outdoor/compressor sensors
  sensor::Sensor *outdoor_temp_sensor_{nullptr};
  sensor::Sensor *condenser_coil_temp_sensor_{nullptr};
  sensor::Sensor *discharge_temp_sensor_{nullptr};
  sensor::Sensor *compressor_freq_sensor_{nullptr};
  sensor::Sensor *indoor_fan_rpm_sensor_{nullptr};
  sensor::Sensor *outdoor_fan_speed_sensor_{nullptr};
  sensor::Sensor *current_amps_sensor_{nullptr};
  text_sensor::TextSensor *heat_mode_active_sensor_{nullptr};
  text_sensor::TextSensor *outdoor_running_sensor_{nullptr};
};

}  // namespace pioneer_minisplit
}  // namespace esphome
