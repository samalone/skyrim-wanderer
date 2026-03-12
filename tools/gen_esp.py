"""Generate a minimal Wanderer.esp for MCM Helper.

Creates an ESL-flagged ESP containing a single quest (WandererMCMQuest) with:
- WandererMCM script attached (extends MCM_ConfigBase)
- PlayerAlias reference alias with SKI_PlayerLoadGameAlias script
- Start Game Enabled flag

Usage: python tools/gen_esp.py
Output: dist/Wanderer.esp
"""

import struct
import os

def wstring(s: str) -> bytes:
    """MCM/VMAD wstring: uint16 length prefix + raw bytes, no null terminator."""
    encoded = s.encode("windows-1252")
    return struct.pack("<H", len(encoded)) + encoded

def zstring(s: str) -> bytes:
    """Null-terminated string."""
    return s.encode("windows-1252") + b"\x00"

def subrecord(tag: str, data: bytes) -> bytes:
    """Build a subrecord: 4-char tag + uint16 size + data."""
    return tag.encode("ascii") + struct.pack("<H", len(data)) + data

def record(tag: str, flags: int, form_id: int, data: bytes) -> bytes:
    """Build a record: 24-byte header + data."""
    return struct.pack(
        "<4sIIIHHHH",
        tag.encode("ascii"),
        len(data),      # data size
        flags,
        form_id,
        0,              # timestamp
        0,              # version control
        44,             # internal version (Skyrim SE)
        0,              # unknown
    ) + data

def grup(label: str, contents: bytes) -> bytes:
    """Build a top-level GRUP (type 0)."""
    size = 24 + len(contents)
    return struct.pack(
        "<4sI4sIHHI",
        b"GRUP",
        size,
        label.encode("ascii"),
        0,              # group type = top level
        0,              # timestamp
        0,              # version control
        0,              # unknown
    ) + contents

def build_vmad() -> bytes:
    """Build the VMAD subrecord data for the quest + alias scripts."""
    data = bytearray()

    # --- Primary script header ---
    data += struct.pack("<hh", 5, 2)    # version=5, objFormat=2
    data += struct.pack("<H", 1)        # scriptCount=1

    # WandererMCM script
    data += wstring("WandererMCM")
    data += struct.pack("<B", 0)        # status=0 (local)
    data += struct.pack("<H", 0)        # propertyCount=0

    # --- Quest fragments section (required even if empty) ---
    data += struct.pack("<b", 2)        # unknown=2
    data += struct.pack("<H", 0)        # fragmentCount=0
    data += wstring("")                 # fileName (empty)

    # --- Alias scripts section ---
    data += struct.pack("<H", 1)        # aliasCount=1

    # Alias 0 object (objFormat=2: uint16 unused, uint16 aliasID, uint32 formID)
    data += struct.pack("<HHI", 0, 0, 0x00000800)

    # Alias 0's own script header
    data += struct.pack("<hh", 5, 2)    # version=5, objFormat=2
    data += struct.pack("<H", 1)        # scriptCount=1

    # SKI_PlayerLoadGameAlias script
    data += wstring("SKI_PlayerLoadGameAlias")
    data += struct.pack("<B", 0)        # status=0
    data += struct.pack("<H", 0)        # propertyCount=0

    return bytes(data)

def build_quest_data() -> bytes:
    """Build all subrecords for the QUST record."""
    parts = bytearray()

    # EDID - Editor ID
    parts += subrecord("EDID", zstring("WandererMCMQuest"))

    # VMAD - Script attachment
    parts += subrecord("VMAD", build_vmad())

    # FULL - Display name
    parts += subrecord("FULL", zstring("Wanderer"))

    # DNAM - Quest data (12 bytes)
    # byte 0: flags1 (0x01 = Start Game Enabled)
    # byte 1: flags2
    # byte 2: priority
    # byte 3: padding
    # bytes 4-7: unknown
    # bytes 8-11: quest type (0 = None)
    dnam = struct.pack("<BBBBI I", 0x01, 0x00, 0x00, 0x00, 0, 0)
    parts += subrecord("DNAM", dnam)

    # ANAM - Next alias ID
    parts += subrecord("ANAM", struct.pack("<I", 1))

    # --- Alias 0: PlayerAlias ---
    # ALST - Alias start (reference alias), index 0
    parts += subrecord("ALST", struct.pack("<I", 0))

    # ALID - Alias name
    parts += subrecord("ALID", zstring("PlayerAlias"))

    # FNAM - Alias flags
    parts += subrecord("FNAM", struct.pack("<I", 0))

    # ALFR - Forced reference (player = 0x14 from Skyrim.esm, master index 0)
    parts += subrecord("ALFR", struct.pack("<I", 0x00000014))

    # VTCK - Voice type (none)
    parts += subrecord("VTCK", struct.pack("<I", 0x00000000))

    # ALED - Alias end marker (empty)
    parts += subrecord("ALED", b"")

    return bytes(parts)

def build_tes4() -> bytes:
    """Build the TES4 file header record."""
    parts = bytearray()

    # HEDR - Header (12 bytes): version 1.7, record count, next object ID
    hedr = struct.pack("<fII", 1.7, 2, 0x801)
    parts += subrecord("HEDR", hedr)

    # CNAM - Author
    parts += subrecord("CNAM", zstring("wanderer"))

    # SNAM - Description
    parts += subrecord("SNAM", zstring("Automatically tracks nearest quest objectives"))

    # MAST + DATA - Master file (Skyrim.esm)
    parts += subrecord("MAST", zstring("Skyrim.esm"))
    parts += subrecord("DATA", struct.pack("<Q", 0))

    flags = 0x00000201  # ESM + ESL
    return record("TES4", flags, 0x00000000, bytes(parts))

def main():
    tes4 = build_tes4()

    quest = record("QUST", 0x00000000, 0x00000800, build_quest_data())
    qust_grup = grup("QUST", quest)

    esp_data = tes4 + qust_grup

    out_dir = os.path.join(os.path.dirname(__file__), "..", "dist")
    out_path = os.path.join(out_dir, "Wanderer.esp")
    os.makedirs(out_dir, exist_ok=True)

    with open(out_path, "wb") as f:
        f.write(esp_data)

    print(f"Generated {out_path} ({len(esp_data)} bytes)")

if __name__ == "__main__":
    main()
