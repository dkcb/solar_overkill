#!/usr/bin/env python
"""Simulated JBD BMS demonstration.

No physical battery or Android device is required. This script wires the
optimized Python backend (bmstools/jbd/jbd.py) to a virtual serial port and a
software BMS that answers the same requests the Overkill Solar mobile app sends
over Bluetooth. It shows:

  - bulk-read packet parsing
  - checksum verification (corrupted frames are rejected)
  - static device-info caching across scan cycles
  - live pack/cell/temperature data
"""

import os
import queue
import struct
import sys
import time

# Make the script runnable from the repo root even under an isolated interpreter.
REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

from bmstools.jbd.jbd import JBD
from bmstools.jbd.parsers import DateParser, TempParser


class SimulatedBMS:
    """Software JBD BMS that replies to read requests over a FakeSerial pipe."""

    REG_BASIC = 0x03
    REG_CELL = 0x04
    REG_DEVICE = 0x05

    START = 0xDD
    END = 0x77

    def __init__(self, cell_count=4, ntc_count=2):
        self.cell_count = cell_count
        self.ntc_count = ntc_count
        self.cycle = 0
        self.device_name = "OKS Sim 4S100A"

    @staticmethod
    def checksum(payload):
        return (0x10000 - sum(payload)) & 0xFFFF

    def _make_frame(self, reg, status, payload):
        length = len(payload)
        body = [status, length] + list(payload)
        chk = self.checksum(body)
        frame = [self.START, reg, status, length] + list(payload) + [(chk >> 8) & 0xFF, chk & 0xFF, self.END]
        return bytes(frame)

    def _basic_payload(self):
        # Slowly move values so the demo looks alive.
        t = time.time()
        current = int(50 + 10 * (1 if int(t) % 10 < 5 else -1))  # +/- 60 mA
        cap_pct = 85 + (self.cycle % 10) - 5

        pack_mv_raw = 3300 + self.cycle                      # 33.0x V after *10
        pack_ma_raw = current                                # 6.0x A after *10
        full_cap_raw = 20000                                 # 200 Ah after *10
        cur_cap_raw = int(full_cap_raw * cap_pct / 100)
        cycle_cnt = self.cycle
        date_raw = DateParser.encode([2025, 7, 4])

        bal_raw = 1                                          # cell 0 balancing
        fault_raw = 0
        version = 10
        fet_raw = 0x03                                       # charge + discharge FETs ON

        ntc_temps = [25.0 + 0.5 * i for i in range(self.ntc_count)]

        payload = struct.pack(
            '>HhHHHH',
            pack_mv_raw,
            pack_ma_raw,
            cur_cap_raw,
            full_cap_raw,
            cycle_cnt,
            date_raw,
        )
        payload += struct.pack(
            '>HHHBBBBB',
            bal_raw & 0xFFFF,
            (bal_raw >> 16) & 0xFFFF,
            fault_raw,
            version,
            cap_pct,
            fet_raw,
            self.cell_count,
            self.ntc_count,
        )
        for temp in ntc_temps:
            payload += struct.pack('>H', TempParser.encode([temp]))
        return payload

    def _cell_payload(self):
        base = 3300
        cells = [base + i * 5 + (self.cycle % 20) for i in range(self.cell_count)]
        return struct.pack(f'>{self.cell_count}H', *cells)

    def _device_payload(self):
        return self.device_name.encode('utf-8')

    def handle_request(self, request):
        """Parse a request frame and return response bytes (or b'' for garbage)."""
        # Need at least start + op + reg + len + checksum2 + end => 7 bytes.
        if len(request) < 7 or request[0] != self.START or request[-1] != self.END:
            return b''

        # Compute request checksum to confirm we only answer valid reads.
        length = request[3]
        if len(request) != 7 + length:
            return b''
        body = request[2:-3]
        recv_chk = (request[-3] << 8) | request[-2]
        if self.checksum(body) != recv_chk:
            return b''

        op = request[1]
        reg = request[2]
        if op != 0xA5:          # not a read operation
            return b''

        self.cycle += 1

        if reg == self.REG_BASIC:
            return self._make_frame(reg, 0x00, self._basic_payload())
        if reg == self.REG_CELL:
            return self._make_frame(reg, 0x00, self._cell_payload())
        if reg == self.REG_DEVICE:
            return self._make_frame(reg, 0x00, self._device_payload())

        return b''


class FakeSerial:
    """In-memory serial pipe that the JBD class can talk to."""

    def __init__(self, bms):
        self.bms = bms
        self.rx = queue.Queue()
        self.timeout = 1.0
        self._open = True

    def open(self):
        self._open = True
        # Discard stale bytes from a previous transaction.
        while not self.rx.empty():
            try:
                self.rx.get_nowait()
            except queue.Empty:
                break

    def close(self):
        self._open = False

    def read(self, size=1):
        out = bytearray()
        deadline = time.time() + self.timeout
        while len(out) < size:
            remaining = deadline - time.time()
            if remaining <= 0:
                break
            try:
                out.append(self.rx.get(timeout=remaining))
            except queue.Empty:
                break
        return bytes(out)

    def write(self, data):
        response = self.bms.handle_request(bytes(data))
        for b in response:
            self.rx.put(b)


def print_dashboard(basic, cell, device):
    print("\n=== Overkill Solar BMS (simulated) ===")
    print(f"Device name : {device.get('device_name')}")
    print(f"Pack V      : {basic.get('pack_mv')} mV")
    print(f"Pack I      : {basic.get('pack_ma')} mA")
    print(f"Capacity    : {basic.get('cap_pct')}% ({basic.get('cur_cap')} / {basic.get('full_cap')} mAh)")
    print(f"Cycles      : {basic.get('cycle_cnt')}")
    print(f"Cells       : {basic.get('cell_cnt')}")
    print(f"MOSFETs     : CHG={basic.get('chg_fet_en')}, DSG={basic.get('dsg_fet_en')}")
    print(f"NTC count   : {basic.get('ntc_cnt')}")
    for i in range(basic.get('ntc_cnt')):
        print(f"  NTC{i+1}      : {basic.get(f'ntc{i}')} C")
    cell_count = basic.get('cell_cnt')
    print("Cell voltages:")
    for i in range(cell_count):
        print(f"  Cell {i+1}    : {cell.get(f'cell{i}_mv')} mV")


def main():
    bms = SimulatedBMS(cell_count=4, ntc_count=2)
    fake = FakeSerial(bms)
    jbd = JBD(fake, timeout=1, debug=False)

    print("Reading device info once, then polling basic + cell info repeatedly...")
    print("(This is the same call pattern the Android app uses during a scan.)")

    # Show that a corrupted frame is rejected.
    print("\n--- corrupt-frame test ---")
    bad_frame = bytes([0xDD, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77])
    fake.write(bad_frame)       # Push garbage directly into the pipe.
    ok, payload = jbd.readPacket()
    print(f"Corrupt frame accepted? {ok} (expected: False)")

    # Normal scan loop
    print("\n--- simulated scan polling ---")
    device_cache = None
    for scan in range(3):
        basic = jbd.readBasicInfo()
        cell = jbd.readCellInfo()
        if device_cache is None:
            device_cache = jbd.readDeviceInfo()
        print_dashboard(basic, cell, device_cache)
        time.sleep(0.2)

    print(f"\nCompleted {bms.cycle} simulated BMS transaction cycles.")


if __name__ == '__main__':
    main()
