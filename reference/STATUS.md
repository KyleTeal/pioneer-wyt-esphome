# Protocol Status

What's working, what's not, and what's still being figured out.

## Working

| Feature | RX Byte | Notes |
|---------|---------|-------|
| Power | 7 bit 4 | |
| Mode | 7 bits 0-3 | Cool=1, Fan=2, Dry=3, Heat=4, Auto=5 |
| Display | 7 bit 5 | |
| Eco | 7 bit 6 | |
| Turbo/Strong | 7 bit 7 | |
| Fan Speed | 8 bits 4-7 | Auto=8, Low=9, Med=A, High=B |
| Set Temp | 8 bits 0-3 | value + 16 = °C |
| Health/Ion | 9 bit 2 | |
| Timer Active | 9 bit 6 | |
| Swing V Active | 10 bit 6 | |
| Swing H Active | 10 bit 5 | |
| Current Temp | 17-18 | BE16 / 374 → °F |
| Sleep Mode | 19 | 0x88=off, 0x89-8B=modes |
| 8°C Heater | 32 bit 7 | |
| Mute | 33 bit 7 | |
| Swing V Position | 51 | |
| Swing H Position | 52 | |
| Outdoor Running | 40 bits 0-3 | 0x0A=running, 0x00=idle |
| Heat Mode Active | 40 bit 6 | |

## Still Investigating

| Byte | What I've Seen | Best Guess |
|------|----------------|------------|
| 30 | 0x6E idle, rises to 0x82-0x8C when heating | Indoor coil temp (÷4 = °C) |
| 34 | 0x3C idle, jumps to 0x50 under load | Operating state, not a temp |
| 35 | 0x1F-0x20 | Minor flags |
| 36-37 | Drift over time | Runtime counters or outdoor feedback |

## Notes

### The "room temperature" isn't room temperature

The indoor temp sensor (bytes 17-18) matches what the Tuya app shows, but it's measuring return air at the unit - not actual room temp. If you put a thermometer across the room, it'll read different. That's normal.

### Beep is TX-only

The HVAC doesn't report beep state back. You can send the command but can't read current state.

### TX and RX mode values are different

Heat is 0x01 in TX but 0x04 in RX. See PROTOCOL.md for the full mapping.
