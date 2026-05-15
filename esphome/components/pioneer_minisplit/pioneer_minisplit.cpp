#include "pioneer_minisplit.h"
#include "esphome/core/log.h"
#include "esphome/core/version.h"
#include "esphome/core/preferences.h"

namespace esphome {
namespace pioneer_minisplit {

static const char *const TAG = "pioneer_minisplit";

void PioneerMinisplit::update_history_(uint8_t *history, uint8_t value) {
  for (int i = 4; i > 0; i--) {
    history[i] = history[i - 1];
  }
  history[0] = value;
}

std::string PioneerMinisplit::format_history_(uint8_t *history) {
  char buf[40];
  snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X",
           history[0], history[1], history[2], history[3], history[4]);
  return std::string(buf);
}

void PioneerMinisplit::setup() {
  ESP_LOGI(TAG, "Pioneer Mini Split Controller Started");
  ESP_LOGI(TAG, "Protocol: 9600 8E1, XOR checksum");
  
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  // ESPHome 2026.4.0+: Set custom modes on entity (new API)
  this->set_supported_custom_fan_modes({"Strong"});
#endif
  
  // Restore swing positions from flash
  this->pref_ = global_preferences->make_preference<SwingState>(this->get_object_id_hash());
  SwingState recovered{};
  if (this->pref_.load(&recovered)) {
    this->pending_swing_v_ = recovered.swing_v;
    this->pending_swing_v_active_ = recovered.swing_v_active;
    this->pending_swing_h_ = recovered.swing_h;
    ESP_LOGI(TAG, "Restored swing state from flash: V=0x%02X active=%d, H=0x%02X", 
             recovered.swing_v, recovered.swing_v_active, recovered.swing_h);
  }
  
  this->send_heartbeat_();
}

void PioneerMinisplit::dump_config() {
  ESP_LOGCONFIG(TAG, "Pioneer Mini Split:");
  ESP_LOGCONFIG(TAG, "  Protocol: 9600 8E1 (Even parity), XOR checksum");
  this->check_uart_settings(9600, 1, uart::UART_CONFIG_PARITY_EVEN, 8);
}

void PioneerMinisplit::loop() {
  uint32_t now = millis();
  
  // Send heartbeat every 3 seconds
  if (now - this->last_heartbeat_ >= HEARTBEAT_INTERVAL) {
    this->send_heartbeat_();
    this->last_heartbeat_ = now;
  }
  
  // Send pending command after cooldown
  if (this->command_pending_ && (now - this->last_command_) >= COMMAND_COOLDOWN) {
    this->send_command_();
    this->command_pending_ = false;
    this->last_command_ = now;
  }
  
  // Read incoming data
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    
    if (this->rx_index_ == 0 && byte != 0xBB) {
      continue;
    }
    
    this->rx_buffer_[this->rx_index_++] = byte;
    this->last_rx_time_ = now;
    
    if (this->rx_index_ >= 5) {
      uint8_t payload_len = this->rx_buffer_[4];
      size_t expected = 5 + payload_len + 1;
      
      if (this->rx_index_ >= expected) {
        this->process_packet_(this->rx_buffer_, this->rx_index_);
        this->rx_index_ = 0;
      }
    }
    
    if (this->rx_index_ >= 64) {
      ESP_LOGW(TAG, "RX buffer overflow");
      this->rx_index_ = 0;
    }
  }
  
  if (this->rx_index_ > 0 && (now - this->last_rx_time_) > RX_TIMEOUT) {
    ESP_LOGD(TAG, "RX timeout, discarding %d bytes", this->rx_index_);
    this->rx_index_ = 0;
  }
}

uint8_t PioneerMinisplit::calculate_checksum_(const uint8_t *data, size_t len) {
  uint8_t xor_sum = 0;
  for (size_t i = 0; i < len; i++) {
    xor_sum ^= data[i];
  }
  return xor_sum;
}

std::string PioneerMinisplit::hex_string_(uint8_t *buf, size_t len) {
  char hex[200];
  size_t pos = 0;
  for (size_t i = 0; i < len && pos < sizeof(hex) - 4; i++) {
    pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", buf[i]);
  }
  return std::string(hex);
}

void PioneerMinisplit::send_heartbeat_() {
  uint8_t packet[] = {0xBB, 0x00, 0x01, 0x04, 0x02, 0x01, 0x00, 0x00};
  packet[7] = this->calculate_checksum_(packet, 7);
  
  this->write_array(packet, sizeof(packet));
  this->flush();
  
  this->tx_count_++;
  if (this->packets_tx_sensor_) this->packets_tx_sensor_->publish_state(this->tx_count_);
  if (this->last_tx_sensor_) this->last_tx_sensor_->publish_state(this->hex_string_(packet, sizeof(packet)));
  
  ESP_LOGD(TAG, "Sent heartbeat");
}

void PioneerMinisplit::send_command_() {
  // Command: BB 00 01 03 1D [29 bytes] [xor] = 35 bytes
  uint8_t packet[35] = {0};
  
  packet[0] = 0xBB;
  packet[1] = 0x00;
  packet[2] = 0x01;
  packet[3] = 0x03;
  packet[4] = 0x1D;  // 29 bytes payload
  
  // Byte 7: flags1 [eco, display, beep, ontimer, offtimer, power, 0, 0]
  uint8_t flags1 = 0;
  if (this->pending_eco_) flags1 |= 0x80;
  if (this->pending_display_) flags1 |= 0x40;
  if (this->pending_beep_) flags1 |= 0x20;
  if (this->pending_power_) flags1 |= 0x04;
  packet[7] = flags1;
  
  // Byte 8: [mute, 0, turbo, health, mode(4)]
  uint8_t mode_flags = this->pending_mode_ & 0x0F;
  if (this->pending_health_) mode_flags |= 0x10;
  if (this->pending_turbo_) mode_flags |= 0x40;
  if (this->pending_mute_) mode_flags |= 0x80;
  packet[8] = mode_flags;
  
  // Byte 9: temp = 111 - setpoint
  packet[9] = 111 - this->pending_temp_;
  
  // Byte 10: fan byte with vertical louver enable bits and 8c heater flag
  // Bits 3-5 (0x38) enable vertical louver motor movement
  uint8_t fan_byte = this->pending_fan_;
  if (this->pending_swing_v_active_) {
    fan_byte |= 0x38;
  } else {
    fan_byte &= 0xC7;
  }
  if (this->pending_8c_heater_) fan_byte |= 0x80;
  packet[10] = fan_byte;
  
  // Byte 11: H swing enable (0x08 when H is actively swinging, not fixed)
  // Auto swing = 0x88, Swing Left = 0x90, Swing Center = 0x98, Swing Right = 0xA0
  if (this->pending_swing_h_ == 0x88 || this->pending_swing_h_ == 0x90 || 
      this->pending_swing_h_ == 0x98 || this->pending_swing_h_ == 0xA0) {
    packet[11] = 0x08;
  }
  
  // Byte 19: sleep mode
  packet[19] = this->pending_sleep_;
  
  // Byte 32: swing_v position
  packet[32] = this->pending_swing_v_;
  
  // Byte 33: swing_h position
  packet[33] = this->pending_swing_h_;
  
  packet[34] = this->calculate_checksum_(packet, 34);
  
  this->write_array(packet, 35);
  this->flush();
  
  this->tx_count_++;
  if (this->packets_tx_sensor_) this->packets_tx_sensor_->publish_state(this->tx_count_);
  if (this->last_tx_sensor_) this->last_tx_sensor_->publish_state(this->hex_string_(packet, sizeof(packet)));
  
  ESP_LOGI(TAG, "TX CMD: pwr=%d mode=0x%02X temp=%d fan=0x%02X",
           this->pending_power_, this->pending_mode_, this->pending_temp_, this->pending_fan_);
}

void PioneerMinisplit::process_packet_(uint8_t *buf, size_t len) {
  uint8_t calc_xor = this->calculate_checksum_(buf, len - 1);
  bool checksum_ok = (calc_xor == buf[len - 1]);
  
  ESP_LOGD(TAG, "RX: %d bytes, cmd=0x%02X%s", len, buf[3], checksum_ok ? "" : " (BAD CSUM)");
  
  this->rx_count_++;
  if (this->packets_rx_sensor_) this->packets_rx_sensor_->publish_state(this->rx_count_);
  if (this->last_rx_sensor_) this->last_rx_sensor_->publish_state(this->hex_string_(buf, len));
  
  // Only decode cmd 0x04 status packets from HVAC (BB 01 00 04 ...)
  if (buf[1] == 0x01 && buf[2] == 0x00 && buf[3] == 0x04 && checksum_ok) {
    this->decode_rx_packet_(buf, len);
  }
}

void PioneerMinisplit::decode_rx_packet_(uint8_t *buf, size_t len) {
  // Same decoding as bb_sniffer
  if (len < 18) return;
  
  uint8_t power_mode = buf[7];
  uint8_t fan_temp = buf[8];
  uint8_t byte9 = buf[9];
  uint8_t byte10 = buf[10];
  uint8_t byte11 = len > 11 ? buf[11] : 0;
  uint8_t byte12 = len > 12 ? buf[12] : 0;
  uint8_t byte18 = len > 18 ? buf[18] : 0;
  uint8_t byte19 = len > 19 ? buf[19] : 0;
  uint8_t byte30 = len > 30 ? buf[30] : 0;
  uint8_t byte32 = len > 32 ? buf[32] : 0;
  uint8_t byte33 = len > 33 ? buf[33] : 0;
  uint8_t byte34 = len > 34 ? buf[34] : 0;
  uint8_t byte35 = len > 35 ? buf[35] : 0;
  uint8_t byte36 = len > 36 ? buf[36] : 0;
  uint8_t byte37 = len > 37 ? buf[37] : 0;
  uint8_t byte50 = len > 51 ? buf[51] : 0;
  uint8_t byte51 = len > 52 ? buf[52] : 0;
  
  bool power = (power_mode & 0x10) != 0;
  bool turbo = (power_mode & 0x80) != 0;
  bool eco = (power_mode & 0x40) != 0;
  bool display = (power_mode & 0x20) != 0;
  uint8_t mode = power_mode & 0x0F;
  
  uint8_t fan = (fan_temp >> 4) & 0x0F;
  uint8_t set_temp = (fan_temp & 0x0F) + 16;
  
  bool timer_active = (byte9 & 0x40) != 0;
  bool health = (byte9 & 0x04) != 0;
  
  bool swing_v_active = (byte10 & 0x40) != 0;
  bool swing_h_active = (byte10 & 0x20) != 0;
  
  uint8_t sleep_mode = 0;
  if (byte19 == 0x89) sleep_mode = 1;
  else if (byte19 == 0x8A) sleep_mode = 2;
  else if (byte19 == 0x8B) sleep_mode = 3;
  
  bool heater_8c = (byte32 & 0x80) != 0;
  bool mute_flag = (byte33 & 0x80) != 0;
  
  // Current temp: bytes 17-18 BE / 374 = °F
  float current_temp = 0;
  float current_temp_f = 0;
  if (len >= 19) {
    uint16_t raw = (uint16_t(buf[17]) << 8) | buf[18];
    if (raw > 0 && raw < 65000) {
      current_temp_f = float(raw) / 374.0f;
      current_temp = (current_temp_f - 32.0f) / 1.8f;
    }
  }
  
  ESP_LOGD(TAG, "RX: %s %s %dC %.1fC fan=%X",
           power ? "ON" : "OFF", this->mode_str_(mode), set_temp, current_temp, fan);
  
  // Update sensors (publish Celsius - HA will convert to user's preferred unit)
  if (this->set_temp_sensor_) this->set_temp_sensor_->publish_state((float)set_temp);
  if (current_temp > 0) {
    if (this->current_temp_sensor_) this->current_temp_sensor_->publish_state(current_temp);
  }
  
  if (len > 30) {
    float coil_temp_c = byte30 / 4.0f;
    if (this->coil_temp_sensor_) this->coil_temp_sensor_->publish_state(coil_temp_c);
  }
  
  if (this->power_sensor_) this->power_sensor_->publish_state(power ? "ON" : "OFF");
  if (this->mode_sensor_) this->mode_sensor_->publish_state(this->mode_str_(mode));
  if (this->fan_sensor_) this->fan_sensor_->publish_state(this->fan_full_str_(fan, turbo, mute_flag));
  
  if (this->swing_sensor_) {
    if (swing_v_active && swing_h_active) this->swing_sensor_->publish_state("Both");
    else if (swing_v_active) this->swing_sensor_->publish_state("Vertical");
    else if (swing_h_active) this->swing_sensor_->publish_state("Horizontal");
    else this->swing_sensor_->publish_state("Off");
  }
  
  if (this->eco_sensor_) this->eco_sensor_->publish_state(eco ? "ON" : "OFF");
  if (this->turbo_sensor_) this->turbo_sensor_->publish_state(turbo ? "ON" : "OFF");
  if (this->display_sensor_) this->display_sensor_->publish_state(display ? "ON" : "OFF");
  if (this->sleep_sensor_) this->sleep_sensor_->publish_state(this->sleep_str_(sleep_mode));
  if (this->health_sensor_) this->health_sensor_->publish_state(health ? "ON" : "OFF");
  if (this->timer_sensor_) this->timer_sensor_->publish_state(timer_active ? "ON" : "OFF");
  if (this->timer_time_sensor_) {
    if (timer_active && (byte11 > 0 || byte12 > 0)) {
      char time_buf[16];
      snprintf(time_buf, sizeof(time_buf), "%dh %dm", byte11, byte12);
      this->timer_time_sensor_->publish_state(time_buf);
    } else {
      this->timer_time_sensor_->publish_state("--");
    }
  }
  if (this->heater_8c_sensor_) this->heater_8c_sensor_->publish_state(heater_8c ? "ON" : "OFF");
  if (this->mute_sensor_) this->mute_sensor_->publish_state(mute_flag ? "ON" : "OFF");
  if (this->beep_sensor_) this->beep_sensor_->publish_state("N/A (TX only)");
  
  if (this->swing_v_sensor_) this->swing_v_sensor_->publish_state(this->swing_v_rx_str_(byte50, swing_v_active));
  if (this->swing_h_sensor_) this->swing_h_sensor_->publish_state(this->swing_h_rx_str_(byte51, swing_h_active));
  
  // Raw byte sensors
  char hex_buf[8];
  if (this->byte9_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte9); this->byte9_sensor_->publish_state(hex_buf); }
  if (this->byte10_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte10); this->byte10_sensor_->publish_state(hex_buf); }
  if (this->byte18_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte18); this->byte18_sensor_->publish_state(hex_buf); }
  if (this->byte19_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte19); this->byte19_sensor_->publish_state(hex_buf); }
  if (this->byte30_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte30); this->byte30_sensor_->publish_state(hex_buf); }
  if (this->byte32_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte32); this->byte32_sensor_->publish_state(hex_buf); }
  if (this->byte33_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte33); this->byte33_sensor_->publish_state(hex_buf); }
  if (this->byte34_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte34); this->byte34_sensor_->publish_state(hex_buf); }
  if (this->byte35_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte35); this->byte35_sensor_->publish_state(hex_buf); }
  if (this->byte36_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte36); this->byte36_sensor_->publish_state(hex_buf); }
  if (this->byte37_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte37); this->byte37_sensor_->publish_state(hex_buf); }
  if (this->byte50_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte50); this->byte50_sensor_->publish_state(hex_buf); }
  if (this->byte51_sensor_) { snprintf(hex_buf, sizeof(hex_buf), "0x%02X", byte51); this->byte51_sensor_->publish_state(hex_buf); }
  
  // History
  this->update_history_(this->byte30_history_, byte30);
  this->update_history_(this->byte34_history_, byte34);
  this->update_history_(this->byte35_history_, byte35);
  this->update_history_(this->byte36_history_, byte36);
  this->update_history_(this->byte37_history_, byte37);
  
  if (this->byte30_history_sensor_) this->byte30_history_sensor_->publish_state(this->format_history_(this->byte30_history_));
  if (this->byte34_history_sensor_) this->byte34_history_sensor_->publish_state(this->format_history_(this->byte34_history_));
  if (this->byte35_history_sensor_) this->byte35_history_sensor_->publish_state(this->format_history_(this->byte35_history_));
  if (this->byte36_history_sensor_) this->byte36_history_sensor_->publish_state(this->format_history_(this->byte36_history_));
  if (this->byte37_history_sensor_) this->byte37_history_sensor_->publish_state(this->format_history_(this->byte37_history_));
  
  // Debug sensors
  if (len > 30) {
    if (this->debug_byte30_f_div4_) this->debug_byte30_f_div4_->publish_state(this->c_to_f_(byte30 / 4.0f));
    if (this->debug_byte30_state_) this->debug_byte30_state_->publish_state((float)byte30);
  }
  if (len > 34) {
    if (this->debug_byte34_f_div4_) this->debug_byte34_f_div4_->publish_state(this->c_to_f_(byte34 / 4.0f));
    if (this->debug_byte34_state_) this->debug_byte34_state_->publish_state((float)byte34);
  }
  if (len > 35) {
    uint16_t p3435 = (uint16_t(buf[34]) << 8) | buf[35];
    if (this->debug_pair_34_35_f_374_) this->debug_pair_34_35_f_374_->publish_state(p3435 / 374.0f);
    if (this->debug_pair_34_35_state_) this->debug_pair_34_35_state_->publish_state((float)p3435);
    if (this->debug_byte35_f_div4_) this->debug_byte35_f_div4_->publish_state(this->c_to_f_(byte35 / 4.0f));
    if (this->debug_byte35_state_) this->debug_byte35_state_->publish_state((float)byte35);
  }
  if (len > 36) {
    uint16_t p3536 = (uint16_t(buf[35]) << 8) | buf[36];
    if (this->debug_pair_35_36_f_374_) this->debug_pair_35_36_f_374_->publish_state(p3536 / 374.0f);
    if (this->debug_pair_35_36_state_) this->debug_pair_35_36_state_->publish_state((float)p3536);
    if (this->debug_byte36_f_div4_) this->debug_byte36_f_div4_->publish_state(this->c_to_f_(byte36 / 4.0f));
    if (this->debug_byte36_state_) this->debug_byte36_state_->publish_state((float)byte36);
  }
  if (len > 37) {
    uint16_t p3637 = (uint16_t(buf[36]) << 8) | buf[37];
    if (this->debug_pair_36_37_f_374_) this->debug_pair_36_37_f_374_->publish_state(p3637 / 374.0f);
    if (this->debug_pair_36_37_state_) this->debug_pair_36_37_state_->publish_state((float)p3637);
    if (this->debug_byte37_f_div4_) this->debug_byte37_f_div4_->publish_state(this->c_to_f_(byte37 / 4.0f));
    if (this->debug_byte37_state_) this->debug_byte37_state_->publish_state((float)byte37);
  }
  
  // Outdoor temp: byte35 - 20 = °C
  if (len > 35) {
    float outdoor_c = buf[35] - 20.0f;
    if (this->outdoor_temp_sensor_) this->outdoor_temp_sensor_->publish_state(outdoor_c);
  }
  
  // Condenser coil temp: byte36 = °C
  if (len > 36) {
    float condenser_c = (float)buf[36];
    if (this->condenser_coil_temp_sensor_) this->condenser_coil_temp_sensor_->publish_state(condenser_c);
  }
  
  // Compressor discharge temp: byte37 = °C
  if (len > 37) {
    float discharge_c = (float)buf[37];
    if (this->discharge_temp_sensor_) this->discharge_temp_sensor_->publish_state(discharge_c);
  }
  
  // Compressor frequency: byte38 = Hz
  if (len > 38) {
    if (this->compressor_freq_sensor_) this->compressor_freq_sensor_->publish_state((float)buf[38]);
  }
  
  // Indoor fan RPM: byte34 (0=off, 60=low, 85=med, 98=high)
  if (len > 34) {
    if (this->indoor_fan_rpm_sensor_) this->indoor_fan_rpm_sensor_->publish_state((float)byte34);
  }
  
  // Outdoor fan speed: byte39
  if (len > 39) {
    if (this->outdoor_fan_speed_sensor_) this->outdoor_fan_speed_sensor_->publish_state((float)buf[39]);
  }
  
  // Current amps: byte46
  if (len > 46) {
    if (this->current_amps_sensor_) this->current_amps_sensor_->publish_state((float)buf[46]);
  }
  
  // Heat mode active: byte40 bit 6
  if (len > 40) {
    bool heat_mode_active = (buf[40] & 0x40) != 0;
    if (this->heat_mode_active_sensor_) this->heat_mode_active_sensor_->publish_state(heat_mode_active ? "ON" : "OFF");
    
    // Outdoor unit status: byte40 bits 0-3 (0x00 = Idle, 0x0A = Running)
    uint8_t outdoor_status = buf[40] & 0x0F;
    if (this->outdoor_running_sensor_) {
      if (outdoor_status == 0x0A) {
        this->outdoor_running_sensor_->publish_state("Running");
      } else if (outdoor_status == 0x00) {
        this->outdoor_running_sensor_->publish_state("Idle");
      } else {
        char buf_str[8];
        snprintf(buf_str, sizeof(buf_str), "0x%02X", outdoor_status);
        this->outdoor_running_sensor_->publish_state(buf_str);
      }
    }
  }
  
  
  // Update climate entity from RX data ONLY
  if (!power) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else {
    // RX mode: 01=Cool, 02=Fan, 03=Dry, 04=Heat, 05=Auto
    switch (mode) {
      case 0x01: this->mode = climate::CLIMATE_MODE_COOL; this->action = climate::CLIMATE_ACTION_COOLING; break;
      case 0x02: this->mode = climate::CLIMATE_MODE_FAN_ONLY; this->action = climate::CLIMATE_ACTION_FAN; break;
      case 0x03: this->mode = climate::CLIMATE_MODE_DRY; this->action = climate::CLIMATE_ACTION_DRYING; break;
      case 0x04: this->mode = climate::CLIMATE_MODE_HEAT; this->action = climate::CLIMATE_ACTION_HEATING; break;
      case 0x05: this->mode = climate::CLIMATE_MODE_HEAT_COOL; this->action = climate::CLIMATE_ACTION_IDLE; break;
      default: this->mode = climate::CLIMATE_MODE_OFF; break;
    }
  }
  
  // Fan mode - Strong is custom, Quiet is standard CLIMATE_FAN_QUIET
  if (turbo && fan == 0x0B) {
    this->set_custom_fan_mode_("Strong");
  } else {
    // Clear custom fan mode when using standard modes
    this->clear_custom_fan_mode_();
    if (mute_flag && fan == 0x09) {
      this->fan_mode = climate::CLIMATE_FAN_QUIET;
    } else {
      switch (fan) {
        case 0x08: this->fan_mode = climate::CLIMATE_FAN_AUTO; break;
        case 0x09: this->fan_mode = climate::CLIMATE_FAN_LOW; break;
        case 0x0A: this->fan_mode = climate::CLIMATE_FAN_MEDIUM; break;
        case 0x0B: this->fan_mode = climate::CLIMATE_FAN_HIGH; break;
        default: this->fan_mode = climate::CLIMATE_FAN_AUTO; break;
      }
    }
  }
  
  // Preset based on eco, turbo, sleep
  if (eco) {
    this->preset = climate::CLIMATE_PRESET_ECO;
  } else if (turbo) {
    this->preset = climate::CLIMATE_PRESET_BOOST;
  } else if (sleep_mode > 0) {
    this->preset = climate::CLIMATE_PRESET_SLEEP;
  } else {
    this->preset = climate::CLIMATE_PRESET_NONE;
  }
  
  // Swing
  if (swing_v_active && swing_h_active) this->swing_mode = climate::CLIMATE_SWING_BOTH;
  else if (swing_v_active) this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  else if (swing_h_active) this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
  else this->swing_mode = climate::CLIMATE_SWING_OFF;
  
  this->target_temperature = set_temp;
  this->current_temperature = current_temp;
  this->publish_state();
  
  // Only sync pending state from RX when no command is pending
  // This prevents RX from overwriting a pending user change before it's sent
  if (!this->command_pending_) {
    this->pending_power_ = power;
    this->pending_temp_ = set_temp;
    this->pending_eco_ = eco;
    this->pending_turbo_ = turbo;
    this->pending_display_ = display;
    this->pending_health_ = health;
    this->pending_mute_ = mute_flag;
    this->pending_8c_heater_ = heater_8c;
    this->pending_swing_v_active_ = swing_v_active;
    
    // Sync mode and fan from RX (convert RX mode to TX mode format)
    // RX: 01=Cool, 02=Fan, 03=Dry, 04=Heat, 05=Auto
    // TX: 01=Heat, 02=Dry, 03=Cool, 07=Fan, 08=Auto
    switch (mode) {
      case 0x01: this->pending_mode_ = 0x03; break;  // RX Cool -> TX Cool
      case 0x02: this->pending_mode_ = 0x07; break;  // RX Fan -> TX Fan
      case 0x03: this->pending_mode_ = 0x02; break;  // RX Dry -> TX Dry
      case 0x04: this->pending_mode_ = 0x01; break;  // RX Heat -> TX Heat
      case 0x05: this->pending_mode_ = 0x08; break;  // RX Auto -> TX Auto
    }
    
    // Sync fan from RX (convert RX fan nibble to TX fan byte)
    // RX: 0x08=Auto, 0x09=Low, 0x0A=Med, 0x0B=High
    // TX: 0x38=Auto, 0x3A=Low, 0x3B=Med, 0x3D=High
    switch (fan) {
      case 0x08: this->pending_fan_ = 0x38; break;
      case 0x09: this->pending_fan_ = 0x3A; break;
      case 0x0A: this->pending_fan_ = 0x3B; break;
      case 0x0B: this->pending_fan_ = 0x3D; break;
    }
    
    // Mark state as synced after first valid RX decode
    this->state_synced_ = true;

    // Update switch states from RX data (only when not waiting to send a command)
    if (this->display_switch_) this->display_switch_->publish_state(display);
    if (this->health_switch_) this->health_switch_->publish_state(health);
    if (this->heater_8c_switch_) this->heater_8c_switch_->publish_state(heater_8c);
    // Note: beep_switch_ state is TX only, can't read from RX
    
    // Update select states from RX data (only when not waiting to send a command)
    if (this->swing_v_select_) {
      this->swing_v_select_->publish_state(this->swing_v_rx_str_(byte50, swing_v_active));
    }
    if (this->swing_h_select_) {
      this->swing_h_select_->publish_state(this->swing_h_rx_str_(byte51, swing_h_active));
    }
  }
}

const char* PioneerMinisplit::mode_str_(uint8_t mode) {
  switch (mode) {
    case 0x01: return "Cool";
    case 0x02: return "Fan";
    case 0x03: return "Dry";
    case 0x04: return "Heat";
    case 0x05: return "Auto";
    default: return "Unknown";
  }
}

const char* PioneerMinisplit::fan_full_str_(uint8_t fan, bool turbo, bool mute) {
  if (fan == 0x0B && turbo) return "Strong";
  if (fan == 0x09 && mute) return "Quiet";
  switch (fan) {
    case 0x08: return "Auto";
    case 0x09: return "Low";
    case 0x0A: return "Medium";
    case 0x0B: return "High";
    case 0x0C: return "Mid-Low";
    case 0x0D: return "Mid-High";
    default: return "Unknown";
  }
}

const char* PioneerMinisplit::sleep_str_(uint8_t sleep) {
  switch (sleep) {
    case 0: return "OFF";
    case 1: return "Standard";
    case 2: return "Elderly";
    case 3: return "Child";
    default: return "Unknown";
  }
}

const char* PioneerMinisplit::swing_v_rx_str_(uint8_t byte50, bool swing_active) {
  // RX byte 50 values mirror TX byte 32 values
  // When swing_active but byte50 is 0, unit is in auto swing mode
  if (byte50 == 0x00 && swing_active) return "Swing - Auto";
  switch (byte50) {
    case 0x00: return "Off";
    case 0x01: return "Fixed Top";
    case 0x02: return "Fixed Upper";
    case 0x03: return "Fixed Middle";
    case 0x04: return "Fixed Lower";
    case 0x05: return "Fixed Bottom";
    case 0x08: return "Swing - Auto";
    case 0x10: return "Swing - Upper";
    case 0x18: return "Swing - Lower";
    default: return swing_active ? "Swing - Auto" : "Off";
  }
}

const char* PioneerMinisplit::swing_h_rx_str_(uint8_t byte51, bool swing_active) {
  // When swing_active but byte51 is 0, unit is in auto swing mode
  if (byte51 == 0x00 && swing_active) return "Swing - Auto";
  switch (byte51) {
    case 0x00: return "Off";
    case 0x01: return "Fixed Far Left";
    case 0x02: return "Fixed Left";
    case 0x03: return "Fixed Center";
    case 0x04: return "Fixed Right";
    case 0x05: return "Fixed Far Right";
    case 0x08: return "Swing - Auto";
    case 0x10: return "Swing - Left";
    case 0x18: return "Swing - Center";
    case 0x20: return "Swing - Right";
    default: return "Off";
  }
}

void PioneerMinisplit::control(const climate::ClimateCall &call) {
  // Don't allow commands until we've synced state from HVAC at least once
  // This prevents sending commands with default/uninitialized values
  if (!this->state_synced_) {
    ESP_LOGW(TAG, "Ignoring command - waiting for initial state sync from HVAC");
    return;
  }
  
  if (call.get_mode().has_value()) {
    climate::ClimateMode new_mode = *call.get_mode();
    if (new_mode == climate::CLIMATE_MODE_OFF) {
      this->pending_power_ = false;
    } else {
      this->pending_power_ = true;
      // TX mode: 01=heat, 02=dry, 03=cool, 07=fan, 08=auto
      switch (new_mode) {
        case climate::CLIMATE_MODE_HEAT: this->pending_mode_ = 0x01; break;
        case climate::CLIMATE_MODE_DRY: this->pending_mode_ = 0x02; break;
        case climate::CLIMATE_MODE_COOL: this->pending_mode_ = 0x03; break;
        case climate::CLIMATE_MODE_FAN_ONLY: this->pending_mode_ = 0x07; break;
        case climate::CLIMATE_MODE_HEAT_COOL: this->pending_mode_ = 0x08; break;
        default: break;
      }
    }
    this->command_pending_ = true;
  }
  
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    if (temp < 16) temp = 16;
    if (temp > 31) temp = 31;
    this->pending_temp_ = (uint8_t)temp;
    this->command_pending_ = true;
  }
  
  // Handle standard fan modes
  if (call.get_fan_mode().has_value()) {
    climate::ClimateFanMode fan = *call.get_fan_mode();
    this->pending_turbo_ = false;
    this->pending_mute_ = false;
    switch (fan) {
      case climate::CLIMATE_FAN_AUTO: this->pending_fan_ = 0x38; break;
      case climate::CLIMATE_FAN_QUIET: 
        this->pending_fan_ = 0x3A; 
        this->pending_mute_ = true; 
        break;
      case climate::CLIMATE_FAN_LOW: this->pending_fan_ = 0x3A; break;
      case climate::CLIMATE_FAN_MEDIUM: this->pending_fan_ = 0x3B; break;
      case climate::CLIMATE_FAN_HIGH: this->pending_fan_ = 0x3D; break;
      default: break;
    }
    this->command_pending_ = true;
  }
  
  // Handle custom fan mode (Strong only - Quiet is now standard CLIMATE_FAN_QUIET)
  if (call.has_custom_fan_mode()) {
    auto custom_fan = call.get_custom_fan_mode();
    this->pending_turbo_ = false;
    this->pending_mute_ = false;
    if (custom_fan == "Strong") {
      this->pending_fan_ = 0x3D;
      this->pending_turbo_ = true;
    }
    this->command_pending_ = true;
  }
  
  // Handle presets (Eco, Boost/Turbo, Sleep)
  if (call.get_preset().has_value()) {
    climate::ClimatePreset preset = *call.get_preset();
    // Clear conflicting presets
    this->pending_eco_ = false;
    this->pending_turbo_ = false;
    this->pending_sleep_ = 0;
    
    switch (preset) {
      case climate::CLIMATE_PRESET_NONE:
        // All cleared above
        break;
      case climate::CLIMATE_PRESET_ECO:
        this->pending_eco_ = true;
        break;
      case climate::CLIMATE_PRESET_BOOST:
        this->pending_turbo_ = true;
        this->pending_fan_ = 0x3D;  // High fan for turbo
        break;
      case climate::CLIMATE_PRESET_SLEEP:
        this->pending_sleep_ = 1;  // Sleep mode 1
        break;
      default:
        break;
    }
    this->command_pending_ = true;
  }
  
  // Swing mode removed from climate - use separate select entities instead

  // Optimistic state update - publish immediately, RX will correct if needed
  if (this->command_pending_) {
    // Update mode from pending values
    if (!this->pending_power_) {
      this->mode = climate::CLIMATE_MODE_OFF;
      this->action = climate::CLIMATE_ACTION_OFF;
    } else {
      // Update mode based on pending_mode_
      switch (this->pending_mode_) {
        case 0x01: this->mode = climate::CLIMATE_MODE_HEAT; break;
        case 0x02: this->mode = climate::CLIMATE_MODE_DRY; break;
        case 0x03: this->mode = climate::CLIMATE_MODE_COOL; break;
        case 0x07: this->mode = climate::CLIMATE_MODE_FAN_ONLY; break;
        case 0x08: this->mode = climate::CLIMATE_MODE_HEAT_COOL; break;
        default: break;
      }
    }
    
    this->target_temperature = this->pending_temp_;
    
    // Update fan mode based on pending state
    if (this->pending_turbo_) {
      this->set_custom_fan_mode_("Strong");
    } else {
      // Clear custom fan mode when switching to standard mode
      this->clear_custom_fan_mode_();
      if (this->pending_mute_) {
        this->fan_mode = climate::CLIMATE_FAN_QUIET;
      } else {
        switch (this->pending_fan_) {
          case 0x38: this->fan_mode = climate::CLIMATE_FAN_AUTO; break;
          case 0x3A: this->fan_mode = climate::CLIMATE_FAN_LOW; break;
          case 0x3B: this->fan_mode = climate::CLIMATE_FAN_MEDIUM; break;
          case 0x3D: this->fan_mode = climate::CLIMATE_FAN_HIGH; break;
          default: this->fan_mode = climate::CLIMATE_FAN_AUTO; break;
        }
      }
    }
    
    // Update preset based on pending state
    if (this->pending_eco_) {
      this->preset = climate::CLIMATE_PRESET_ECO;
    } else if (this->pending_turbo_) {
      this->preset = climate::CLIMATE_PRESET_BOOST;
    } else if (this->pending_sleep_ > 0) {
      this->preset = climate::CLIMATE_PRESET_SLEEP;
    } else {
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
    
    this->publish_state();
  }
}

climate::ClimateTraits PioneerMinisplit::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION | climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_visual_min_temperature(16);
  traits.set_visual_max_temperature(31);
  traits.set_visual_temperature_step(1);
  
  traits.set_supported_modes({
    climate::CLIMATE_MODE_OFF,
    climate::CLIMATE_MODE_COOL,
    climate::CLIMATE_MODE_HEAT,
    climate::CLIMATE_MODE_DRY,
    climate::CLIMATE_MODE_FAN_ONLY,
    climate::CLIMATE_MODE_HEAT_COOL
  });
  
  // Standard fan modes: Auto, Quiet, Low, Medium, High (Strong is custom)
  traits.set_supported_fan_modes({
    climate::CLIMATE_FAN_AUTO,
    climate::CLIMATE_FAN_QUIET,
    climate::CLIMATE_FAN_LOW,
    climate::CLIMATE_FAN_MEDIUM,
    climate::CLIMATE_FAN_HIGH,
  });

#if ESPHOME_VERSION_CODE < VERSION_CODE(2026, 4, 0)
  // Pre-2026.4.0: Set custom fan modes on traits (old API)
  traits.set_supported_custom_fan_modes({"Strong"});
#endif
  // 2026.4.0+: Custom fan modes are set in setup() and wired automatically
  
  // Swing modes removed from climate - use separate select entities instead
  
  // Standard presets
  traits.set_supported_presets({
    climate::CLIMATE_PRESET_NONE,
    climate::CLIMATE_PRESET_ECO,
    climate::CLIMATE_PRESET_BOOST,
    climate::CLIMATE_PRESET_SLEEP,
  });
  
  return traits;
}

// Switch implementation
void PioneerSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) return;
  this->parent_->set_feature(static_cast<SwitchType>(this->type_), state);
  this->publish_state(state);
}

// Select implementation
void PioneerSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) return;
  this->parent_->set_swing_position(static_cast<SelectType>(this->type_), value);
  this->publish_state(value);
}

// Feature control from switches (only for features not covered by climate presets/modes)
void PioneerMinisplit::set_feature(SwitchType type, bool state) {
  if (!this->state_synced_) {
    ESP_LOGW(TAG, "Ignoring switch - waiting for initial state sync from HVAC");
    return;
  }
  
  switch (type) {
    case SWITCH_DISPLAY:
      this->pending_display_ = state;
      break;
    case SWITCH_BEEP:
      this->pending_beep_ = state;
      break;
    case SWITCH_HEALTH:
      this->pending_health_ = state;
      break;
    case SWITCH_HEATER_8C:
      this->pending_8c_heater_ = state;
      break;
  }
  this->command_pending_ = true;
}

// Swing position control from selects
void PioneerMinisplit::set_swing_position(SelectType type, const std::string &value) {
  if (!this->state_synced_) {
    ESP_LOGW(TAG, "Ignoring select - waiting for initial state sync from HVAC");
    return;
  }
  
  if (type == SELECT_SWING_V) {
    // V swing mirrors H swing pattern but without the 0x80 base
    // H: Off=0x80, Auto=0x88, Left=0x90, Center=0x98, Right=0xA0, Fixed=0x81-0x85
    // V: Off=0x00, Auto=0x08, Upper=0x10, Lower=0x18, Fixed=0x01-0x05
    if (value == "Off") {
      this->pending_swing_v_active_ = false;
      this->pending_swing_v_ = 0x00;
    } else if (value == "Swing - Auto") {
      this->pending_swing_v_active_ = true;
      this->pending_swing_v_ = 0x08;
    } else if (value == "Swing - Upper") {
      this->pending_swing_v_active_ = true;
      this->pending_swing_v_ = 0x10;
    } else if (value == "Swing - Lower") {
      this->pending_swing_v_active_ = true;
      this->pending_swing_v_ = 0x18;
    } else if (value == "Fixed Top") {
      this->pending_swing_v_active_ = false;
      this->pending_swing_v_ = 0x01;
    } else if (value == "Fixed Upper") {
      this->pending_swing_v_active_ = false;
      this->pending_swing_v_ = 0x02;
    } else if (value == "Fixed Middle") {
      this->pending_swing_v_active_ = false;
      this->pending_swing_v_ = 0x03;
    } else if (value == "Fixed Lower") {
      this->pending_swing_v_active_ = false;
      this->pending_swing_v_ = 0x04;
    } else if (value == "Fixed Bottom") {
      this->pending_swing_v_active_ = false;
      this->pending_swing_v_ = 0x05;
    }
    // Immediately publish the selected value to the select
    if (this->swing_v_select_) {
      this->swing_v_select_->publish_state(value);
    }
  } else if (type == SELECT_SWING_H) {
    if (value == "Off") {
      this->pending_swing_h_ = 0x80;
    } else if (value == "Swing - Auto") {
      this->pending_swing_h_ = 0x88;
    } else if (value == "Swing - Left") {
      this->pending_swing_h_ = 0x90;
    } else if (value == "Swing - Center") {
      this->pending_swing_h_ = 0x98;
    } else if (value == "Swing - Right") {
      this->pending_swing_h_ = 0xA0;
    } else if (value == "Fixed Far Left") {
      this->pending_swing_h_ = 0x81;
    } else if (value == "Fixed Left") {
      this->pending_swing_h_ = 0x82;
    } else if (value == "Fixed Center") {
      this->pending_swing_h_ = 0x83;
    } else if (value == "Fixed Right") {
      this->pending_swing_h_ = 0x84;
    } else if (value == "Fixed Far Right") {
      this->pending_swing_h_ = 0x85;
    }
    // Immediately publish the selected value to the select
    if (this->swing_h_select_) {
      this->swing_h_select_->publish_state(value);
    }
  }
  
  // Save swing state to flash
  SwingState state{};
  state.swing_v = this->pending_swing_v_;
  state.swing_v_active = this->pending_swing_v_active_;
  state.swing_h = this->pending_swing_h_;
  this->pref_.save(&state);
  
  this->command_pending_ = true;
}

}  // namespace pioneer_minisplit
}  // namespace esphome
