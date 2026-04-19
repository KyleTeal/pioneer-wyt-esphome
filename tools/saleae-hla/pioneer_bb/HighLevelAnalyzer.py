"""
Pioneer / WYT mini split BB-protocol High Level Analyzer for Saleae Logic 2.

Consumes byte frames from an Async Serial analyzer (9600 8E1 -- EVEN parity,
not 8N1) attached to the controller<->indoor unit UART line and emits
decoded high-level frames.

Goal: PASSIVE STATUS LISTENING. The primary deliverable is the `state` row,
emitted once whenever any "data point we care about" changes inside a status
response (cmd 0x04 from HVAC). Each row carries one column per byte plus
decoded interpretations -- this is the table you cross-reference against
the panel display / Tuya app to pin down byte->meaning mappings, then port
into the ESPHome component.

Frame structure (see docs/PROTOCOL.md):

  [HEADER] [DIR] [DIR2] [CMD] [LEN] [PAYLOAD...] [CHECKSUM]
     BB     XX    XX     XX    XX    ...           XX

  HEADER : 0xBB (sync byte)
  DIR    : 0x00 = controller -> HVAC, 0x01 = HVAC -> controller
  DIR2   : 0x01 (to HVAC) or 0x00 (from HVAC)
  CMD    : 0x04 = heartbeat / status, 0x03 = set command, ...
  LEN    : length of PAYLOAD (NOT including header/dir/cmd/len/checksum)
  CSUM   : XOR of every byte before it

Filterable types (Logic 2 -> filter by `type=...`):

  - state     : ONE row per change to any "stable setting" byte of a
                STATUS RESPONSE (BB 01 00 04 ... from HVAC). Watched bytes
                are the user-visible settings that DON'T drift on their own
                (power/mode/fan/temp/timer/sleep/health/8c/mute). All
                payload bytes are still shown as columns + decoded fields.
                Filter to type=state for the cross-reference table.
  - status    : every status response (cmd 0x04 from HVAC), even unchanged.
                Chatty (~one per heartbeat, ~3s). Use when you need to
                inspect the "drifting" bytes (current temp, coil temps,
                runtime counters, amps, fan rpm).
  - heartbeat : the ESP->HVAC poll (BB 00 01 04 02 01 00 XX). Tiny, cheap.
  - command   : ESP->HVAC set command (BB 00 01 03 1C ... 28-byte payload).
                Decoded with TX byte semantics.
  - other     : any well-formed BB frame whose CMD we don't recognize.
  - bad_csum  : frame parsed cleanly but checksum mismatch (wire glitch).
  - raw       : framing didn't line up (header missing / truncated). Rare.

Workflow:
  1. Capture a full session with the panel/app, annotated with what you did.
  2. Filter to type=state.
  3. Cross-reference each row's byte columns against the panel state at
     that moment. Refine docs/PROTOCOL.md.
  4. Port any new mappings into esphome/components/pioneer_minisplit/.
"""

from saleae.analyzers import (
    HighLevelAnalyzer,
    AnalyzerFrame,
    NumberSetting,
)


# -- Protocol constants ------------------------------------------------------

BB_HEADER = 0xBB

DIR_TO_HVAC = (0x00, 0x01)    # bytes 1,2 of an ESP->HVAC frame
DIR_FROM_HVAC = (0x01, 0x00)  # bytes 1,2 of an HVAC->ESP frame

CMD_NAMES = {
    0x03: "SetCommand",
    0x04: "Status/Heartbeat",
}

# Status RX byte indices (absolute into the full frame buffer, matching
# pioneer_minisplit.cpp). Frame layout puts payload[0] at buf[5], so
# "byte 7" in the protocol doc == buf[7] == payload[2]. We keep the same
# numbering as the existing C++ decoder so columns line up with PROTOCOL.md.
RX_STATUS_LAYOUT = {
    7: "power_mode",      # bit7=Turbo bit6=Eco bit5=Display bit4=Power bits0-3=Mode
    8: "fan_temp",        # bits4-7=Fan bits0-3=(Temp - 16)
    9: "timer_health",    # bit6=Timer bit2=Health
    10: "swing_flags",    # bit6=SwingV active bit5=SwingH active
    17: "cur_temp_hi",    # BE16 / 374 = degF
    18: "cur_temp_lo",
    19: "sleep_mode",     # 0x88..0x8B
    30: "indoor_coil_q4", # /4 = degC (suspect)
    32: "heater_8c",      # bit7
    33: "mute",           # bit7
    34: "indoor_fan_rpm", # 0=off,60=low,85=med,98=high
    35: "outdoor_temp",   # byte - 20 = degC
    36: "condenser_c",    # = degC
    37: "discharge_c",    # = degC
    38: "comp_freq_hz",
    39: "outdoor_fan",
    40: "heat_state",     # bit6=heat-mode-active bits0-3=outdoor-status (0x0A=run)
    46: "current_amps",
    51: "swing_v_pos",
    52: "swing_h_pos",
}

# Bytes that change on USER ACTION only -- watch these for the `state` row.
# Excludes everything that drifts continuously (temps, fan rpm, amps,
# runtime counters, op-state). b8 contains both fan and setpoint so it's
# included; current temp (17/18) is NOT.
RX_STATUS_WATCH_INDICES = (7, 8, 9, 10, 19, 32, 33, 51, 52)

# How many byte columns to expose per row (we cap because Logic 2 columns
# get unwieldy past ~16). We emit b0..b15 always for short frames, then
# additionally expose the named decoded columns regardless of position.
BYTE_TABLE_COLUMNS = 16

# RX mode (HVAC -> controller) -- different numbering from TX mode.
RX_MODE_NAMES = {
    0x01: "Cool",
    0x02: "Fan",
    0x03: "Dry",
    0x04: "Heat",
    0x05: "Auto",
}

# RX fan (high nibble of byte 8).
RX_FAN_NAMES = {
    0x8: "Auto",
    0x9: "Low",
    0xA: "Medium",
    0xB: "High",
}

RX_SLEEP_NAMES = {
    0x00: "Off",
    0x88: "Off",
    0x89: "Standard",
    0x8A: "Elderly",
    0x8B: "Child",
}

# TX mode (controller -> HVAC), sent in payload of cmd 0x03.
TX_MODE_NAMES = {
    0x01: "Heat",
    0x02: "Dry",
    0x03: "Cool",
    0x07: "Fan",
    0x08: "Auto",
}

TX_FAN_NAMES = {
    0x38: "Auto",
    0x3A: "Low",
    0x3B: "Medium",
    0x3D: "High",
}

SWING_V_POS = {
    0x00: "Off",
    0x01: "Fixed1(Top)",
    0x02: "Fixed2",
    0x03: "Fixed3(Mid)",
    0x04: "Fixed4",
    0x05: "Fixed5(Bottom)",
    0x08: "AutoSwing",
    0x10: "SwingUpper",
    0x18: "SwingLower",
}

SWING_H_POS_RX = {
    0x00: "Off",
    0x01: "Fixed1(FarLeft)",
    0x02: "Fixed2",
    0x03: "Fixed3(Center)",
    0x04: "Fixed4",
    0x05: "Fixed5(FarRight)",
    0x08: "AutoSwing",
    0x10: "SwingLeft",
    0x18: "SwingCenter",
    0x20: "SwingRight",
}


# -- Decoders ---------------------------------------------------------------


def _hex_str(buf):
    return " ".join(f"{b:02X}" for b in buf)


def _byte_columns(buf):
    """b0..b15 hex columns, blank past the end of the buffer."""
    out = {}
    for i in range(BYTE_TABLE_COLUMNS):
        out[f"b{i}"] = f"0x{buf[i]:02X}" if i < len(buf) else ""
    return out


def _xor(buf):
    x = 0
    for b in buf:
        x ^= b
    return x


def _decode_rx_status(buf):
    """Return a dict of decoded fields for an HVAC->controller cmd 0x04.
    Mirrors the field interpretation in pioneer_minisplit.cpp::decode_rx_packet_."""
    n = len(buf)
    g = lambda i: buf[i] if i < n else None

    out = {}

    b7 = g(7)
    if b7 is not None:
        out["power"] = "ON" if (b7 & 0x10) else "OFF"
        out["mode"] = RX_MODE_NAMES.get(b7 & 0x0F, f"0x{b7 & 0x0F:02X}")
        out["turbo"] = "1" if (b7 & 0x80) else "0"
        out["eco"] = "1" if (b7 & 0x40) else "0"
        out["display"] = "1" if (b7 & 0x20) else "0"

    b8 = g(8)
    if b8 is not None:
        fan_nib = (b8 >> 4) & 0x0F
        out["fan"] = RX_FAN_NAMES.get(fan_nib, f"0x{fan_nib:X}")
        out["set_temp_c"] = str((b8 & 0x0F) + 16)

    b9 = g(9)
    if b9 is not None:
        out["timer_active"] = "1" if (b9 & 0x40) else "0"
        out["health"] = "1" if (b9 & 0x04) else "0"

    b10 = g(10)
    if b10 is not None:
        out["swing_v_act"] = "1" if (b10 & 0x40) else "0"
        out["swing_h_act"] = "1" if (b10 & 0x20) else "0"

    b17, b18 = g(17), g(18)
    if b17 is not None and b18 is not None:
        raw = (b17 << 8) | b18
        if 0 < raw < 65000:
            f_temp = raw / 374.0
            out["cur_temp_f"] = f"{f_temp:.1f}"
            out["cur_temp_c"] = f"{(f_temp - 32.0) / 1.8:.1f}"

    b19 = g(19)
    if b19 is not None:
        out["sleep"] = RX_SLEEP_NAMES.get(b19, f"0x{b19:02X}")

    b30 = g(30)
    if b30 is not None:
        out["indoor_coil_c"] = f"{b30 / 4.0:.1f}"

    b32 = g(32)
    if b32 is not None:
        out["heater_8c"] = "1" if (b32 & 0x80) else "0"

    b33 = g(33)
    if b33 is not None:
        out["mute"] = "1" if (b33 & 0x80) else "0"

    b34 = g(34)
    if b34 is not None:
        out["indoor_fan_rpm"] = str(b34)

    b35 = g(35)
    if b35 is not None:
        out["outdoor_c"] = str(b35 - 20)

    b36 = g(36)
    if b36 is not None:
        out["condenser_c"] = str(b36)

    b37 = g(37)
    if b37 is not None:
        out["discharge_c"] = str(b37)

    b38 = g(38)
    if b38 is not None:
        out["comp_hz"] = str(b38)

    b39 = g(39)
    if b39 is not None:
        out["outdoor_fan"] = str(b39)

    b40 = g(40)
    if b40 is not None:
        out["heat_active"] = "1" if (b40 & 0x40) else "0"
        outd = b40 & 0x0F
        out["outdoor_state"] = "Run" if outd == 0x0A else ("Idle" if outd == 0x00 else f"0x{outd:02X}")

    b46 = g(46)
    if b46 is not None:
        out["amps"] = str(b46)

    b51 = g(51)
    if b51 is not None:
        out["swing_v_pos"] = SWING_V_POS.get(b51, f"0x{b51:02X}")

    b52 = g(52)
    if b52 is not None:
        out["swing_h_pos"] = SWING_H_POS_RX.get(b52, f"0x{b52:02X}")

    return out


def _decode_tx_command(buf):
    """Decode a controller->HVAC cmd 0x03 set-command frame using TX byte
    semantics from PROTOCOL.md."""
    n = len(buf)
    g = lambda i: buf[i] if i < n else None
    out = {}

    b7 = g(7)
    if b7 is not None:
        out["power"] = "ON" if (b7 & 0x04) else "OFF"
        out["eco"] = "1" if (b7 & 0x80) else "0"
        out["display"] = "1" if (b7 & 0x40) else "0"
        out["beep"] = "1" if (b7 & 0x20) else "0"

    b8 = g(8)
    if b8 is not None:
        out["mode"] = TX_MODE_NAMES.get(b8 & 0x0F, f"0x{b8 & 0x0F:02X}")
        out["mute"] = "1" if (b8 & 0x80) else "0"
        out["turbo"] = "1" if (b8 & 0x40) else "0"
        out["health"] = "1" if (b8 & 0x10) else "0"

    b9 = g(9)
    if b9 is not None:
        out["set_temp_c"] = str(111 - b9)

    b10 = g(10)
    if b10 is not None:
        out["heater_8c"] = "1" if (b10 & 0x80) else "0"
        # Mask off the 8c flag before looking up the fan code.
        out["fan"] = TX_FAN_NAMES.get(b10 & 0x7F, f"0x{b10 & 0x7F:02X}")

    b19 = g(19)
    if b19 is not None:
        out["sleep"] = str(b19)

    b31 = g(31)
    if b31 is not None:
        out["swing_v_pos"] = SWING_V_POS.get(b31, f"0x{b31:02X}")

    b32 = g(32)
    if b32 is not None:
        # TX horizontal often has bit7 set as enable; show raw + masked.
        out["swing_h_pos"] = f"0x{b32:02X}"

    return out


def _merge(buf, hex_str, extra):
    """Per-byte columns first, then full hex, then decoded fields."""
    data = {**_byte_columns(buf), "bytes": hex_str, "len": str(len(buf))}
    data.update(extra)
    return data


# -- Analyzer ---------------------------------------------------------------


class PioneerBB(HighLevelAnalyzer):
    inter_byte_gap_ms = NumberSetting(
        label="Inter-byte gap (ms) - bytes farther apart than this force a flush "
              "(safety net; framing is normally length-driven)",
        min_value=1,
        max_value=500,
    )

    result_types = {
        # PRIMARY output. Fires once per change to any of the watched
        # "stable setting" bytes inside an HVAC->controller status
        # response. Includes all per-byte columns + decoded settings.
        "state": {
            "format": "STATE  {{data.change}}   "
                      "pwr={{data.power}} mode={{data.mode}} fan={{data.fan}} "
                      "set={{data.set_temp_c}}C cur={{data.cur_temp_c}}C "
                      "sleep={{data.sleep}} swV={{data.swing_v_pos}} swH={{data.swing_h_pos}}"
        },
        # Every status response, even when unchanged. Chatty (~3s).
        "status": {
            "format": "STATUS pwr={{data.power}} mode={{data.mode}} fan={{data.fan}} "
                      "set={{data.set_temp_c}}C cur={{data.cur_temp_c}}C "
                      "outd={{data.outdoor_c}}C cond={{data.condenser_c}}C "
                      "disc={{data.discharge_c}}C hz={{data.comp_hz}} "
                      "ofan={{data.outdoor_fan}} A={{data.amps}} "
                      "ostate={{data.outdoor_state}} heat={{data.heat_active}}"
        },
        "heartbeat": {
            "format": "HB ESP->HVAC {{data.bytes}}"
        },
        "command": {
            "format": "CMD ESP->HVAC pwr={{data.power}} mode={{data.mode}} "
                      "set={{data.set_temp_c}}C fan={{data.fan}} sleep={{data.sleep}} "
                      "swV={{data.swing_v_pos}} swH={{data.swing_h_pos}} "
                      "(eco={{data.eco}} turbo={{data.turbo}} mute={{data.mute}} "
                      "disp={{data.display}} health={{data.health}} 8c={{data.heater_8c}})"
        },
        "other": {
            "format": "FRAME dir={{data.dir}} cmd={{data.cmd}}({{data.cmd_name}}) len={{data.len}} {{data.bytes}}"
        },
        "bad_csum": {
            "format": "BAD_CSUM dir={{data.dir}} cmd={{data.cmd}} got={{data.csum_got}} want={{data.csum_want}} {{data.bytes}}"
        },
        "raw": {
            "format": "RAW {{data.bytes}}"
        },
    }

    # ------------------------------------------------------------------

    def __init__(self):
        try:
            self._gap_s = float(self.inter_byte_gap_ms) / 1000.0
            if self._gap_s <= 0:
                self._gap_s = 0.050
        except Exception:
            self._gap_s = 0.050

        self._buf = []
        self._buf_start = None
        self._buf_last_end = None
        self._expected_len = None  # total frame length once we know LEN

        # Last seen tuple of watched bytes, so we can fire `state` only on
        # change. There's only one status source on the bus, so a single
        # slot is enough (no per-side keying like the Whirlpool decoder).
        self._last_watched = None

    # ------------------------------------------------------------------
    # Saleae entry point
    # ------------------------------------------------------------------

    def decode(self, frame):
        if frame.type != "data":
            return None

        raw = frame.data.get("data")
        if raw is None or len(raw) == 0:
            return None
        byte = raw[0] if isinstance(raw, (bytes, bytearray)) else int(raw)

        emitted = None

        # Safety-net: gap-based flush. Normally framing is length-driven,
        # but if we got desynced (e.g. a stray byte before BB) this lets
        # us recover instead of stalling forever.
        if self._buf and self._buf_last_end is not None:
            gap = float(frame.start_time - self._buf_last_end)
            if gap > self._gap_s:
                emitted = self._flush_raw(end_time=self._buf_last_end)

        # Resync: if we have no buffer and the byte isn't BB, drop it
        # (emit a one-byte raw so it shows up in Logic 2 instead of
        # silently disappearing).
        if not self._buf:
            if byte != BB_HEADER:
                return AnalyzerFrame(
                    "raw", frame.start_time, frame.end_time,
                    _merge([byte], f"{byte:02X}", {"note": "stray-pre-header"}),
                ) if emitted is None else emitted
            self._start(byte, frame)
            return emitted

        # Append to buffer.
        self._buf.append(byte)
        self._buf_last_end = frame.end_time

        # Once we have LEN (buf[4]), compute expected total length:
        # 5 header bytes + payload + 1 checksum.
        if self._expected_len is None and len(self._buf) >= 5:
            self._expected_len = 5 + self._buf[4] + 1

        if self._expected_len is not None and len(self._buf) >= self._expected_len:
            forced = self._flush_frame(end_time=frame.end_time)
            return forced if emitted is None else emitted

        # Hard cap so a malformed LEN can't run away.
        if len(self._buf) >= 80:
            forced = self._flush_raw(end_time=frame.end_time)
            return forced if emitted is None else emitted

        return emitted

    # ------------------------------------------------------------------
    # Internals
    # ------------------------------------------------------------------

    def _start(self, byte, frame):
        self._buf = [byte]
        self._buf_start = frame.start_time
        self._buf_last_end = frame.end_time
        self._expected_len = None

    def _take(self):
        buf = self._buf
        start = self._buf_start
        self._buf = []
        self._buf_start = None
        self._buf_last_end = None
        self._expected_len = None
        return buf, start

    def _flush_raw(self, end_time):
        """Emit whatever's in the buffer as a raw frame (recovery path)."""
        buf, start = self._take()
        if not buf:
            return None
        return AnalyzerFrame(
            "raw", start, end_time,
            _merge(buf, _hex_str(buf), {"note": "framing-desync"}),
        )

    def _flush_frame(self, end_time):
        buf, start = self._take()
        return self._classify(buf, start, end_time)

    def _classify(self, buf, start, end):
        n = len(buf)
        hex_str = _hex_str(buf)

        if n < 6:
            return AnalyzerFrame(
                "raw", start, end,
                _merge(buf, hex_str, {"note": "short"}),
            )

        dir_pair = (buf[1], buf[2])
        cmd = buf[3]
        cmd_name = CMD_NAMES.get(cmd, f"0x{cmd:02X}")
        csum_got = buf[-1]
        csum_want = _xor(buf[:-1])
        dir_label = (
            "ESP->HVAC" if dir_pair == DIR_TO_HVAC
            else "HVAC->ESP" if dir_pair == DIR_FROM_HVAC
            else f"{buf[1]:02X}/{buf[2]:02X}"
        )

        common = {
            "dir": dir_label,
            "cmd": f"0x{cmd:02X}",
            "cmd_name": cmd_name,
            "csum_got": f"0x{csum_got:02X}",
            "csum_want": f"0x{csum_want:02X}",
        }

        if csum_got != csum_want:
            return AnalyzerFrame(
                "bad_csum", start, end,
                _merge(buf, hex_str, common),
            )

        # Heartbeat: tiny ESP->HVAC poll.
        if dir_pair == DIR_TO_HVAC and cmd == 0x04:
            return AnalyzerFrame(
                "heartbeat", start, end,
                _merge(buf, hex_str, common),
            )

        # Set command: ESP->HVAC cmd 0x03 (~34 bytes).
        if dir_pair == DIR_TO_HVAC and cmd == 0x03:
            decoded = _decode_tx_command(buf)
            return AnalyzerFrame(
                "command", start, end,
                _merge(buf, hex_str, {**common, **decoded}),
            )

        # Status response: HVAC->ESP cmd 0x04. THE one we care about.
        if dir_pair == DIR_FROM_HVAC and cmd == 0x04:
            decoded = _decode_rx_status(buf)
            watched = tuple(buf[i] if i < n else None for i in RX_STATUS_WATCH_INDICES)

            change_parts = []
            if self._last_watched is None:
                change_parts.append("first")
            else:
                for idx, prev, cur in zip(RX_STATUS_WATCH_INDICES, self._last_watched, watched):
                    if prev != cur:
                        ps = "??" if prev is None else f"{prev:02X}"
                        cs = "??" if cur is None else f"{cur:02X}"
                        change_parts.append(f"b{idx}({ps}->{cs})")
            is_change = (self._last_watched is None) or (watched != self._last_watched)
            self._last_watched = watched

            decoded["change"] = ",".join(change_parts) if change_parts else ""

            kind = "state" if is_change else "status"
            return AnalyzerFrame(
                kind, start, end,
                _merge(buf, hex_str, {**common, **decoded}),
            )

        # Anything else with valid checksum: surface it so the operator
        # notices new CMD codes.
        return AnalyzerFrame(
            "other", start, end,
            _merge(buf, hex_str, common),
        )
