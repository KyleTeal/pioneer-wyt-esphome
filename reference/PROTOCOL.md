# BB Protocol Reference

Canonical protocol documentation for Pioneer / TCL-style mini splits using the BB serial protocol over UART (9600 8E1). **Do not maintain a second copy** — `docs/PROTOCOL.md` redirects here.

## Serial Config

```
Baud:   9600
Data:   8 bits
Parity: Even (8E1) ← not 8N1
Stop:   1 bit
```

## Packet Structure

```
[HEADER] [DIR] [DIR2] [CMD] [LEN] [PAYLOAD...] [CHECKSUM]
   BB     XX    XX     XX    XX    ...           XX
```

| Field | Size | Description |
|-------|------|-------------|
| HEADER | 1 | Always 0xBB |
| DIR | 1 | 0x00 = to HVAC, 0x01 = from HVAC |
| DIR2 | 1 | 0x01 (to HVAC), 0x00 (from HVAC) |
| CMD | 1 | Command type |
| LEN | 1 | Payload length |
| PAYLOAD | N | Variable |
| CHECKSUM | 1 | XOR of all bytes |

---

## TX Commands (ESP → HVAC)

### Heartbeat (0x04)

8 bytes. Sent every ~3 seconds to poll status.

```
BB 00 01 04 02 01 00 [checksum]
```

### Set Command (0x03)

35 bytes total (`BB 00 01 03 1D` then 29-byte payload plus XOR checksum).

**Key bytes:**

| Byte | Bits | What |
|------|------|------|
| 7 | 7 | Eco |
| 7 | 6 | Display |
| 7 | 5 | Beep |
| 7 | 2 | Power |
| 8 | 7 | Mute |
| 8 | 6 | Turbo |
| 8 | 4 | Health/Ion |
| 8 | 0-3 | Mode |
| 9 | - | Temp: `111 - setpoint_celsius` |
| 10 | 7 | 8 °C heater (manual **46 °F freeze protection**, same BB bit) |
| 10 | 3-5 | Vertical sweep enable (`0x38` while sweeping) |
| 10 | 0-2 | Fan speed encoding |
| 11 | — | Horizontal sweep: `0x08` when left/center/right/auto sweep TX values |
| 19 | - | Sleep mode (0-3) |
| 32 | - | Vertical swing position |
| 33 | - | Horizontal swing position |
| 34 | - | Checksum |

### TX Mode Values

| Value | Mode | Tuya String |
|-------|------|-------------|
| 0x01 | Heat | "hot" |
| 0x02 | Dry | "wet" |
| 0x03 | Cool | "cold" |
| 0x07 | Fan Only | "wind" |
| 0x08 | Auto | "auto" |

### TX Fan Values

Values are `0x38 + offset`.

| Value | Speed | Tuya String | Offset |
|-------|-------|-------------|--------|
| 0x38 | Auto | "auto" | 0x00 |
| 0x3A | Low | "low" | 0x02 |
| 0x3B | Medium | "mid" | 0x03 |
| 0x3D | High | "high" | 0x05 |
| 0x3E | Mid-Low | "mid_low" | 0x06 |
| 0x3F | Mid-High | "mid_high" | 0x07 |
| 0x40 | Strong/Turbo | "strong" | 0x08 |
| 0x41 | Mute/Quiet | "mute" | 0x09 |

**Note:** TX Auto uses 0x38. The Turbo flag in byte 8 bit 6 distinguishes Strong from Auto.

---

## RX Status (HVAC → ESP)

### Status Response (0x04)

~61 bytes. Sent in response to heartbeat.

**Key bytes:**

| Byte | Bits | What |
|------|------|------|
| 7 | 7 | Turbo |
| 7 | 6 | Eco |
| 7 | 5 | Display |
| 7 | 4 | Power |
| 7 | 0-3 | Mode |
| 8 | 4-7 | Fan speed (upper nibble) |
| 8 | 0-3 | Temp offset (+16 = °C) |
| 9 | 6 | Timer active |
| 9 | 2 | Health/Ion |
| 10 | 6 | Swing V active |
| 10 | 5 | Swing H active |
| 17-18 | - | Current temp (BE16 / 374 = °F) |
| 19 | - | Sleep mode |
| 32 | 7 | 8 °C heater / 46 °F freeze protection |
| 33 | 7 | Mute |
| 35 | - | Outdoor temp (byte - 20 = °C) |
| 36 | - | Condenser coil temp (°C) |
| 37 | - | Compressor discharge temp (°C) |
| 38 | - | Compressor frequency (Hz) |
| 39 | - | Outdoor fan speed |
| 40 | 6 | Heat mode active |
| 40 | 0-3 | Outdoor unit status (0x0A = running) |
| 46 | - | Current draw (A) |
| 51 | - | Swing V position |
| 52 | - | Swing H position |

### RX Mode Values

**Different from TX!**

| Value | Mode | Tuya String |
|-------|------|-------------|
| 0x01 | Cool | "cold" |
| 0x02 | Fan Only | "wind" |
| 0x03 | Dry | "wet" |
| 0x04 | Heat | "hot" |
| 0x05 | Auto | "auto" |

### RX Fan Values

Upper nibble of byte 8.

| Upper Nibble | Speed | Tuya String | Raw Value |
|--------------|-------|-------------|-----------|
| 0x8 | Auto | "auto" | 0x00 |
| 0x9 | Low | "low" | 0x01 |
| 0xA | Medium | "mid" | 0x02 |
| 0xB | High | "high" | 0x03 |
| 0xC | Mid-Low | "mid_low" | 0x04 |
| 0xD | Mid-High | "mid_high" | 0x05 |

**Note:** RX byte 8 upper nibble = `0x8 + raw_value`.

---

## Feature Mapping

Quick reference for TX vs RX encoding:

| Feature | TX Byte | TX Value | RX Byte | RX Value |
|---------|---------|----------|---------|----------|
| Power | 7 | bit 2 | 7 | bit 4 |
| Mode | 8 | bits 0-3 | 7 | bits 0-3 |
| Set Temp | 9 | 111 - °C | 8 | low nibble + 16 |
| Fan | 10 | bits 0-2 | 8 | bits 4-7 |
| Display | 7 | bit 6 | 7 | bit 5 |
| Eco | 7 | bit 7 | 7 | bit 6 |
| Turbo | 8 | bit 6 | 7 | bit 7 |
| Mute | 8 | bit 7 | 33 | bit 7 |
| Health | 8 | bit 4 | 9 | bit 2 |
| 8 °C heater / 46 °F freeze | 10 | bit 7 | 32 | bit 7 |
| Sleep | 19 | 0-3 | 19 | `0x89`/`0x8A`/`0x8B` or `0xB1`/`0xB2`/`0xB3` |

YAML: **`heater_8c_switch`** or **`freeze_protection_switch`** (not both — same mechanism). **`heater_8c`** text sensor = RX flag.

### Sleep modes (Tuya DP 105)

Firmware strings map to ESPHome **`sleep_select`**: Off / Standard / Elderly / Child (`normal`→Standard, `old`→Elderly, `child`→Child).

| Mode | TX byte 19 | RX (examples) |
|------|------------|---------------|
| Off | 0 | (not sleeping) |
| Standard | 1 | `0x89` or `0xB1` |
| Elderly | 2 | `0x8A` or `0xB2` |
| Child | 3 | `0x8B` or `0xB3` |

Climate **Sleep** preset sets Standard only (`pending_sleep_` = 1). Full DP 105 range is exposed via **`sleep_select`** when configured.

---

## Swing Positions

Swing conversion uses arithmetic:
- **Mode values:** `tuya_value × 8` (e.g., "1" → 0x08, "2" → 0x10)
- **Position values:** `tuya_value` directly (e.g., "3" → 0x03)
- **H-swing:** Add `0x80` bit to all values

### Vertical (TX byte 32, RX byte 51)

| Tuya DP Value | BB Value | Position |
|---------------|----------|----------|
| "0" | 0x00 | Off |
| "1" (mode) | 0x08 | Auto Swing (Full) |
| "2" (mode) | 0x10 | Swing Upper Zone |
| "3" (mode) | 0x18 | Swing Lower Zone |
| "1" (pos) | 0x01 | Fixed Top |
| "2" (pos) | 0x02 | Fixed Upper |
| "3" (pos) | 0x03 | Fixed Middle |
| "4" (pos) | 0x04 | Fixed Lower |
| "5" (pos) | 0x05 | Fixed Bottom |

**Formula:** Mode = `atoi(dp_string) << 3`, Position = `atoi(dp_string)`

Sweeping uses byte 10 bits 3-5 (`0x38`); clear them when using fixed louvers only.

### Horizontal (TX byte 33, RX byte 52)

All H-swing values have bit 7 set (OR with 0x80).

| Tuya DP Value | BB Value | Position |
|---------------|----------|----------|
| "0" | 0x80 | Off |
| "1" (mode) | 0x88 | Auto Swing (Full) |
| "2" (mode) | 0x90 | Swing Left Zone |
| "3" (mode) | 0x98 | Swing Center Zone |
| "4" (mode) | 0xA0 | Swing Right Zone |
| "1" (pos) | 0x81 | Fixed Far Left |
| "2" (pos) | 0x82 | Fixed Left |
| "3" (pos) | 0x83 | Fixed Center |
| "4" (pos) | 0x84 | Fixed Right |
| "5" (pos) | 0x85 | Fixed Far Right |

**Formula:** Mode = `(atoi(dp_string) << 3) | 0x80`, Position = `atoi(dp_string) | 0x80`

TX byte 11 = `0x08` while horizontal sweep values are active (auto / zone sweep).

### Tuya DP Mapping

| DP ID | Function | Type | Values |
|-------|----------|------|--------|
| 113 | V-Swing Mode | String | "0"=off, "1"=full, "2"=upper, "3"=lower |
| 114 | H-Swing Mode | String | "0"=off, "1"=full, "2"-"4"=zones |
| 126 | V-Swing Position | String | "0"=off, "1"-"5"=positions |
| 127 | H-Swing Position | String | "0"=off, "1"-"5"=positions |

---

## Temperature Formulas

```c
// Set temp (TX)
tx_byte9 = 111 - setpoint_celsius;

// Set temp (RX)
set_temp_c = (byte8 & 0x0F) + 16;

// Current temp (matches Tuya app)
uint16_t raw = (buf[17] << 8) | buf[18];
current_temp_f = raw / 374.0f;
current_temp_c = (current_temp_f - 32.0f) / 1.8f;

// Outdoor temp
outdoor_temp_c = buf[35] - 20.0f;
```

---

## Checksum

XOR all bytes except the checksum itself:

```c
uint8_t checksum(uint8_t *data, size_t len) {
    uint8_t result = 0;
    for (size_t i = 0; i < len - 1; i++) {
        result ^= data[i];
    }
    return result;
}
```


---

## Unknown Bytes

Still figuring these out:

| Byte | Observed | Guess |
|------|----------|-------|
| 30 | 0x6E idle, 0x82+ heating | Indoor coil temp (÷4 = °C) |
| 34 | 0x3C → 0x50 under load | Operating state |
| 36-37 | Drift over time | Runtime counters |
