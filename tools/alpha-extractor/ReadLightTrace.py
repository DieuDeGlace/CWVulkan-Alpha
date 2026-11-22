"""Reads an AVLT light trace written by the Alpha extractor (LightTrace.cpp).

python ReadLightTrace.py lighttrace.avlt            summary by kind and caller
python ReadLightTrace.py lighttrace.avlt --events   every event, one line each
python ReadLightTrace.py lighttrace.avlt --probe K  only events where probe K changed
"""
from collections import Counter
import struct
import sys

KINDS = {1: "light-enter", 2: "light-precommit", 3: "light-exit", 4: "admit-yes", 5: "admit-no",
         6: "build-locked", 7: "relight-chosen", 8: "build-light-done", 9: "step-test",
         10: "cell-write", 11: "cell-write-exit", 12: "clear", 13: "clear-stored",
         14: "capture-begin", 15: "capture-end", 16: "probes-changed"}
CALLERS = {0x09DB89: "mesh", 0x1EDE89: "new-zone", 0x20F076: "bomb", 0x2118E1: "building",
           0x213FC5: "variable-radius"}
UNKNOWN = -2**31
EVENT = struct.Struct("<IHHIIq8i32s")


def read(path):
    data = open(path, "rb").read()
    assert data[:4] == b"AVLT", "not an AVLT file"
    version, stamp, base, count = struct.unpack_from("<4I", data, 4)
    assert version == 2, version
    offset = 20
    probes = []  # one list of (X, Y, Z) per probe set
    for _ in range(count):
        (n,) = struct.unpack_from("<I", data, offset)
        probes.append([struct.unpack_from("<3i", data, offset + 4 + 12 * k) for k in range(n)])
        offset += 4 + 12 * n
    (frequency,) = struct.unpack_from("<q", data, offset)
    claimed, dropped, incomplete, written = struct.unpack_from("<4I", data, offset + 8)
    offset += 24
    assert len(data) == offset + EVENT.size * written, "truncated"
    events = [EVENT.unpack_from(data, offset + EVENT.size * i) for i in range(written)]
    events.sort(key=lambda e: e[0])
    header = dict(stamp=stamp, base=base, probes=probes, frequency=frequency, claimed=claimed,
                  dropped=dropped, incomplete=incomplete, written=written)
    return header, events


def probe_bytes(event, k):
    mask, cells = event[2], event[14]  # seq, kind, mask, thread, probe set, qpc, v[8], probe bytes
    return cells[4 * k:4 * k + 4] if mask >> k & 1 else None


def describe(event, header):
    seq, kind, mask, thread, probe_set, qpc, *rest = event
    values = list(rest[:8])
    name = KINDS.get(kind, str(kind))
    shown = ["?" if v == UNKNOWN else str(v) for v in values]
    if kind in (1, 2, 3) and values[0] != UNKNOWN:
        shown[0] = CALLERS.get(values[0], hex(values[0]))
    probes = " ".join(probe_bytes(event, k).hex() if probe_bytes(event, k) else "--"
                      for k in range(len(header["probes"][probe_set])))
    ms = 1000.0 * qpc / header["frequency"]
    return f"{seq:8d} {ms:14.3f} t{thread:<6d} {name:16s} {' '.join(shown):60s} | s{probe_set} {probes}"


def main():
    header, events = read(sys.argv[1])
    mode = sys.argv[2] if len(sys.argv) > 2 else ""
    if mode == "--events":
        for e in events:
            print(describe(e, header))
        return
    if mode == "--probe":
        k = int(sys.argv[3])
        last = None
        for e in events:
            now = (e[4], probe_bytes(e, k))
            if now != last:
                print(describe(e, header))
                last = now
        return
    print({k: v for k, v in header.items() if k != "frequency"})
    print(Counter(KINDS.get(e[1], e[1]) for e in events))
    print(Counter(CALLERS.get(e[6], hex(e[6])) for e in events if e[1] == 1))
    print("threads", Counter(e[3] for e in events))


if __name__ == "__main__":
    main()
