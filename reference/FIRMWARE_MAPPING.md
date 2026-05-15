# Pioneer Mini-Split Firmware Mapping Analysis

**Source:** `tywe1s_backup_full.bin` (2 MiB flash dump)  
**Base Address:** `0x40200000` (ESP8266 SPI flash mapping)  
**Architecture:** Xtensa (ESP8266)

## Firmware-Extracted Lookup Tables

These tables were found at file offsets `0x12b0-0x13a0` and represent the
definitive Tuya DP string ↔ BB protocol byte mappings.

### TX Mode (Tuya String → BB Protocol Byte)

*Table location: 0x1300-0x1324*


| Tuya String | BB Value | Description |
| ----------- | -------- | ----------- |
| `"auto"`    | `0x08`   | Auto mode   |
| `"cold"`    | `0x03`   | Cooling     |
| `"wet"`     | `0x02`   | Dry         |
| `"wind"`    | `0x07`   | Fan Only    |
| `"hot"`     | `0x01`   | Heating     |


### RX Mode (BB Protocol Byte → Tuya String)

*Table location: 0x1368-0x138c*


| BB Value | Tuya String | Description |
| -------- | ----------- | ----------- |
| `0x01`   | `"cold"`    | Cooling     |
| `0x02`   | `"wind"`    | Fan Only    |
| `0x03`   | `"wet"`     | Dry         |
| `0x04`   | `"hot"`     | Heating     |
| `0x05`   | `"auto"`    | Auto mode   |


### TX Fan Speed (Tuya String → BB Protocol Byte)

*Table location: 0x12c8-0x12fc*

**Note:** These values appear to be offsets. Actual TX byte may be `0x38 + value`.


| Tuya String  | Table Value | Likely TX Byte | Description |
| ------------ | ----------- | -------------- | ----------- |
| `"low"`      | `0x02`      | `0x3A`         | Low         |
| `"mid"`      | `0x03`      | `0x3B`         | Medium      |
| `"high"`     | `0x05`      | `0x3D`         | High        |
| `"mid_low"`  | `0x06`      | `0x3E`         | Medium-Low  |
| `"mid_high"` | `0x07`      | `0x3F`         | Medium-High |
| `"strong"`   | `0x08`      | `0x40`         | Turbo       |
| `"mute"`     | `0x09`      | `0x41`         | Quiet       |


### RX Fan Speed (BB Protocol Byte → Tuya String)

*Table location: 0x132c-0x1364*


| BB Value | Tuya String  | Description |
| -------- | ------------ | ----------- |
| `0x01`   | `"low"`      | Low         |
| `0x02`   | `"mid"`      | Medium      |
| `0x03`   | `"high"`     | High        |
| `0x04`   | `"mid_low"`  | Medium-Low  |
| `0x05`   | `"mid_high"` | Medium-High |
| `0x08`   | `"strong"`   | Turbo       |
| `0x09`   | `"mute"`     | Quiet       |


---

## Comparison with PROTOCOL.md

### Mode Values


| Mode | PROTOCOL.md TX | Firmware TX | PROTOCOL.md RX | Firmware RX |
| ---- | -------------- | ----------- | -------------- | ----------- |
| Heat | 0x01           | 0x01        | 0x04           | 0x04        |
| Dry  | 0x02           | 0x02        | 0x03           | 0x03        |
| Cool | 0x03           | 0x03        | 0x01           | 0x01        |
| Fan  | 0x07           | 0x07        | 0x02           | 0x02        |
| Auto | 0x08           | 0x08        | 0x05           | 0x05        |

Mode TX/RX values match between PROTOCOL.md and firmware tables above.

### Fan Speed Values


| Speed    | PROTOCOL.md TX | Firmware TX (base+offset) | PROTOCOL.md RX | Firmware RX |
| -------- | -------------- | ------------------------- | -------------- | ----------- |
| Auto     | 0x38           | 0x38 (base)               | 0x8            | ?           |
| Low      | 0x3A           | 0x3A (0x38+2)             | 0x9            | 0x1 (diff.) |
| Medium   | 0x3B           | 0x3B (0x38+3)             | 0xA            | 0x2 (diff.) |
| High     | 0x3D           | 0x3D (0x38+5)             | 0xB            | 0x3 (diff.) |
| Mid-Low  | (missing)      | 0x3E (0x38+6)             | (missing)      | 0x4         |
| Mid-High | (missing)      | 0x3F (0x38+7)             | (missing)      | 0x5         |
| Strong   | (missing)      | 0x40 (0x38+8)             | (missing)      | 0x8         |
| Mute     | (missing)      | 0x41 (0x38+9)             | (missing)      | 0x9         |

RX fan speed: firmware lookup table uses low-level enum indices; PROTOCOL documents upper-nibble RX encoding—see discrepancy section below.

---

## Discrepancies Found

### 1. RX Fan Speed Encoding

- **PROTOCOL.md says:** RX fan is upper nibble: `0x8`=Auto, `0x9`=Low, `0xA`=Med, `0xB`=High
- **Firmware shows:** Raw values: `0x1`=Low, `0x2`=Mid, `0x3`=High, etc.

**Possible explanations:**

1. The firmware table may be for internal enum indexing, not raw BB bytes
2. There may be a byte transformation (nibble shift) not captured in the table
3. PROTOCOL.md may be wrong

### 2. Missing Fan Speeds

PROTOCOL.md doesn't document:

- `mid_low` (Medium-Low)
- `mid_high` (Medium-High)
- `strong` (Turbo)
- `mute` (Quiet)

The firmware supports **7 fan speeds**, not 4.

---

## Swing Position Mapping

**Source:** [tuya-local daizuki_heatpump.yaml](https://github.com/make-all/tuya-local/blob/main/custom_components/tuya_local/devices/daizuki_heatpump.yaml)  
**Confirmed via:** [GitHub Issue #820](https://github.com/make-all/tuya-local/issues/820) for Pioneer WYT

### DP 113: Vertical Swing Mode

Controls whether V-swing is active and which zone.


| Tuya Value | Meaning    | BB Protocol Byte |
| ---------- | ---------- | ---------------- |
| `"0"`      | Off        | `0x00` (?)       |
| `"1"`      | Full sweep | `0x08` (?)       |
| `"2"`      | Upper zone | TBD              |
| `"3"`      | Lower zone | TBD              |


### DP 114: Horizontal Swing Mode

Controls whether H-swing is active and which zone.


| Tuya Value | Meaning        | BB Protocol Byte |
| ---------- | -------------- | ---------------- |
| `"0"`      | Off            | `0x00` (?)       |
| `"1"`      | Full sweep     | `0x08` (?)       |
| `"2"`      | Left zone      | TBD              |
| `"3"`      | Center zone    | TBD              |
| `"4"`      | Right zone     | TBD              |


### DP 126: Vertical Fixed Position

Sets a fixed V-position (no sweep).


| Tuya Value | Meaning       | BB Protocol Byte |
| ---------- | ------------- | ---------------- |
| `"0"`      | Unknown/Off   | `0x00`           |
| `"1"`      | Top           | `0x01`           |
| `"2"`      | Slightly up   | `0x02`           |
| `"3"`      | Middle        | `0x03`           |
| `"4"`      | Slightly down | `0x04`           |
| `"5"`      | Bottom        | `0x05`           |


### DP 127: Horizontal Fixed Position

Sets a fixed H-position (no sweep).


| Tuya Value | Meaning      | BB Protocol Byte         |
| ---------- | ------------ | ------------------------ |
| `"0"`      | Unknown/Off  | `0x00` (V) / `0x80` (H)  |
| `"1"`      | Leftmost     | `0x01` + `0x80` = `0x81` |
| `"2"`      | Slight Left  | `0x02` + `0x80` = `0x82` |
| `"3"`      | Center       | `0x03` + `0x80` = `0x83` |
| `"4"`      | Slight Right | `0x04` + `0x80` = `0x84` |
| `"5"`      | Rightmost    | `0x05` + `0x80` = `0x85` |


**Note:** H-swing BB values have bit 7 set (`0x80`) to distinguish from V-swing.

---

## Tuya Data Points (Full List)

DPs found in firmware and confirmed via [GitHub Issue #820](https://github.com/make-all/tuya-local/issues/820):


| DP ID | Name             | Type     | Values / Notes                                 |
| ----- | ---------------- | -------- | ---------------------------------------------- |
| 1     | Power            | Bool     | On/Off                                         |
| 2     | Temperature Set  | Int      | x10 in Fahrenheit (720 = 72°F)                 |
| 3     | Current Temp     | Int      | Celsius                                        |
| 4     | Mode             | Enum     | cold/hot/wet/wind/auto                         |
| 5     | Fan Speed        | Enum     | auto/mute/low/mid_low/mid/mid_high/high/strong |
| 18    | Humidity         | Int      | %                                              |
| 20    | Error Code       | Bitfield | Fault codes                                    |
| 101   | PM2.5            | Int      | μg/m³                                          |
| 105   | Sleep Mode       | Enum     | off/normal/old/child                           |
| 110   | Identification   | Bitfield | Device flags                                   |
| 113   | V-Swing Mode     | String   | "0"=off, "1"=full, "2"=upper, "3"=lower        |
| 114   | H-Swing Mode     | String   | "0"=off, "1"=full, "2"-"4"=zones               |
| 119   | Eco Mode         | String   | Energy management                              |
| 120   | Gen Mode         | String   |                                                |
| 123   | Display/Beep/etc | Hex      | Bitfield: 0x0008=display, 0x0010=beep          |
| 125   | Air Quality      | String   | "great", etc.                                  |
| 126   | V-Position Fixed | String   | "0"-"5" positions                              |
| 127   | H-Position Fixed | String   | "0"-"5" positions                              |
| 128   | Model Code       | String   | Device model                                   |
| 129   | Energy           | String   | kWh counter                                    |
| 130   | Eco Temperature  | Int      |                                                |
| 131   | Filter Dirty     | Bool     | Maintenance alert                              |
| 132   | Hot/Cool Wind    | Bool     |                                                |
| 133   | Swing Action     | String   | Current swing state                            |
| 134   | Statistics       | JSON     | Runtime stats {"t":epoch,"s":bool}             |


---

## Source Files Identified


| File           | Address Range | Purpose                 |
| -------------- | ------------- | ----------------------- |
| dev_uart.c     | ~0x5cb00      | UART driver             |
| dev_protocol.c | ~0x5b000      | Modern protocol handler |
| old_protocal.c | ~0x5d400      | Legacy protocol handler |
| minic.c        | ~0x5da00      | Mini controller         |


---

## Key Protocol Strings


| Address  | String                            | Purpose                 |
| -------- | --------------------------------- | ----------------------- |
| 0x02bc74 | `[ERR]%s:%d wind spd:%d is err!`  | Fan speed validation    |
| 0x02bc94 | `[ERR]%s:%d work mode:%d is err!` | Mode validation         |
| 0x02bccc | `[D]%s:%d cmd:%d len:%d`          | Packet framing log      |
| 0x02bdb0 | `[D]%s:%d temper:%d`              | Temperature debug       |
| 0x02be60 | `[ERR]%s:%d set temper:%d is err` | Temperature validation  |
| 0x02bf84 | `[ERR]%s:%d xor:%x check:%x`      | XOR checksum validation |
| 0x02c080 | `[D]%s:%d uart proc init succ!`   | UART init success       |


---

## Validation Required

To resolve the RX fan speed discrepancy, capture actual BB protocol traffic
with a logic analyzer and compare:

1. The raw bytes sent/received
2. How they map to the Tuya app's displayed fan speed

The firmware extraction provides the Tuya side mapping, but the actual
BB wire format may involve additional transformations.