# BB Protocol Reference

Protocol documentation for mini splits using the BB serial protocol over UART (9600 8E1).

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


| Field    | Size                | Description                      |
| -------- | ------------------- | -------------------------------- |
| HEADER   | 1                   | Always 0xBB                      |
| DIR      | 1                   | 0x00 = to HVAC, 0x01 = from HVAC |
| DIR2     | 1                   | 0x01 (to HVAC), 0x00 (from HVAC) |
| CMD      | 1                   | Command type                     |
| LEN      | 1                   | Payload length                   |
| PAYLOAD  | N                   | Variable                         |
| CHECKSUM | 1                   | XOR of all bytes                 |


---

## TX Commands (ESP → HVAC)

### Heartbeat (0x04)

8 bytes. Sent every ~3 seconds to poll status.

```
BB 00 01 04 02 01 00 [checksum]
```

### Set Command (0x03)

35 bytes total. Sent when changing settings.

**Key bytes:**


| Byte | Bits | What                           |
| ---- | ---- | ------------------------------ |
| 7    | 7    | Eco                            |
| 7    | 6    | Display                        |
| 7    | 5    | Beep                           |
| 7    | 2    | Power                          |
| 8    | 7    | Mute                           |
| 8    | 6    | Turbo                          |
| 8    | 4    | Health/Ion                     |
| 8    | 0-3  | Mode                           |
| 9    | -    | Temp: `111 - setpoint_celsius` |
| 10   | 7    | 8°C Heater                     |
| 10   | 3-5  | Vertical sweep enable (`0x38` while sweeping) |
| 10   | 0-2  | Fan speed                      |
| 11   | —    | Horizontal sweep: `0x08` when using auto/zone sweep TX values |
| 19   | -    | Sleep mode (0-3)               |
| 32   | -    | Vertical swing position        |
| 33   | -    | Horizontal swing position      |
| 34   | -    | Checksum (XOR of bytes 0..33)  |


**TX Mode values:**


| Value | Mode     |
| ----- | -------- |
| 0x01  | Heat     |
| 0x02  | Dry      |
| 0x03  | Cool     |
| 0x07  | Fan Only |
| 0x08  | Auto     |


**TX Fan values:**


| Value | Speed  |
| ----- | ------ |
| 0x38  | Auto   |
| 0x3A  | Low    |
| 0x3B  | Medium |
| 0x3D  | High   |


---

## RX Status (HVAC → ESP)

### Status Response (0x04)

~61 bytes. Sent in response to heartbeat.

**Key bytes:**


| Byte  | Bits | What                                 |
| ----- | ---- | ------------------------------------ |
| 7     | 7    | Turbo                                |
| 7     | 6    | Eco                                  |
| 7     | 5    | Display                              |
| 7     | 4    | Power                                |
| 7     | 0-3  | Mode                                 |
| 8     | 4-7  | Fan speed                            |
| 8     | 0-3  | Temp offset (+16 = °C)               |
| 9     | 6    | Timer active                         |
| 9     | 2    | Health/Ion                           |
| 10    | 6    | Swing V active                       |
| 10    | 5    | Swing H active                       |
| 17-18 | -    | Current temp (BE16 / 374 = °F)       |
| 19    | -    | Sleep mode                           |
| 32    | 7    | 8°C Heater                           |
| 33    | 7    | Mute                                 |
| 35    | -    | Outdoor temp (byte - 20 = °C)        |
| 36    | -    | Condenser coil temp (°C)             |
| 37    | -    | Compressor discharge temp (°C)       |
| 38    | -    | Compressor frequency (Hz)            |
| 39    | -    | Outdoor fan speed                    |
| 40    | 6    | Heat mode active                     |
| 40    | 0-3  | Outdoor unit status (0x0A = running) |
| 46    | -    | Current draw (A)                     |
| 51    | -    | Swing V position                     |
| 52    | -    | Swing H position                     |


**RX Mode values (different from TX!):**


| Value | Mode     |
| ----- | -------- |
| 0x01  | Cool     |
| 0x02  | Fan Only |
| 0x03  | Dry      |
| 0x04  | Heat     |
| 0x05  | Auto     |


**RX Fan values:**


| Nibble | Speed  |
| ------ | ------ |
| 0x8    | Auto   |
| 0x9    | Low    |
| 0xA    | Medium |
| 0xB    | High   |


---

## Feature Mapping

Quick reference for TX vs RX encoding:


| Feature    | TX Byte | TX Value | RX Byte | RX Value        |
| ---------- | ------- | -------- | ------- | --------------- |
| Power      | 7       | bit 2    | 7       | bit 4           |
| Mode       | 8       | bits 0-3 | 7       | bits 0-3        |
| Set Temp   | 9       | 111 - °C | 8       | low nibble + 16 |
| Fan        | 10      | bits 0-2 | 8       | bits 4-7        |
| Display    | 7       | bit 6    | 7       | bit 5           |
| Eco        | 7       | bit 7    | 7       | bit 6           |
| Turbo      | 8       | bit 6    | 7       | bit 7           |
| Mute       | 8       | bit 7    | 33      | bit 7           |
| Health     | 8       | bit 4    | 9       | bit 2           |
| 8°C Heater | 10      | bit 7    | 32      | bit 7           |
| Sleep      | 19      | 0-3      | 19      | 0x88-0x8B       |


---

## Swing Positions

### Vertical (TX byte 32, RX byte 51)

Sweeping: set TX byte 10 bits 3-5 (`0x38`). Fixed louvers: clear those bits (`fan_byte & ~0x38`). Same role as byte 11 for horizontal sweep.


| TX   | RX   | Position         |
| ---- | ---- | ---------------- |
| 0x00 | 0x00 | Off              |
| 0x08 | 0x08 | Auto Swing       |
| 0x10 | 0x10 | Swing Upper      |
| 0x18 | 0x18 | Swing Lower      |
| 0x01 | 0x01 | Fixed 1 (Top)    |
| 0x02 | 0x02 | Fixed 2          |
| 0x03 | 0x03 | Fixed 3 (Middle) |
| 0x04 | 0x04 | Fixed 4          |
| 0x05 | 0x05 | Fixed 5 (Bottom) |


### Horizontal (TX byte 33, RX byte 52)


| TX   | RX   | Position            |
| ---- | ---- | ------------------- |
| 0x80 | 0x00 | Off                 |
| 0x88 | 0x08 | Auto Swing          |
| 0x90 | 0x10 | Swing Left          |
| 0x98 | 0x18 | Swing Center        |
| 0xA0 | 0x20 | Swing Right         |
| 0x81 | 0x01 | Fixed 1 (Far Left)  |
| 0x82 | 0x02 | Fixed 2             |
| 0x83 | 0x03 | Fixed 3 (Center)    |
| 0x84 | 0x04 | Fixed 4             |
| 0x85 | 0x05 | Fixed 5 (Far Right) |


---

## Temperature Formulas

```c
// Set temp
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


| Byte  | Observed                 | Guess                      |
| ----- | ------------------------ | -------------------------- |
| 30    | 0x6E idle, 0x82+ heating | Indoor coil temp (÷4 = °C) |
| 34    | 0x3C → 0x50 under load   | Operating state            |
| 36-37 | Drift over time          | Runtime counters           |


