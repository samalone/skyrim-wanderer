"""Compile WandererMCM.psc using the Papyrus compiler.

Usage: python tools/gen_pex.py
Output: dist/Scripts/WandererMCM.pex
"""

import subprocess
import sys
import os

COMPILER = os.path.join(
    "G:", os.sep, "Steam", "steamapps", "common",
    "Skyrim Special Edition 1946180", "Papyrus Compiler", "PapyrusCompiler.exe"
)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
IMPORT_DIR = os.path.join(SCRIPT_DIR, "papyrus-sources")
SOURCE_FILE = os.path.join(IMPORT_DIR, "WandererMCM.psc")
OUTPUT_DIR = os.path.join(PROJECT_DIR, "dist", "Scripts")


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Ensure source is in import dir (compiler needs it there)
    psc_src = os.path.join(PROJECT_DIR, "dist", "Scripts", "Source", "WandererMCM.psc")
    if os.path.exists(psc_src):
        import shutil
        shutil.copy2(psc_src, SOURCE_FILE)

    if not os.path.exists(COMPILER):
        print(f"Error: Papyrus compiler not found at {COMPILER}", file=sys.stderr)
        sys.exit(1)

    cmd = [
        COMPILER,
        SOURCE_FILE,
        f"-i={IMPORT_DIR}",
        f"-o={OUTPUT_DIR}",
        "-f=TESV_Papyrus_Flags.flg",
    ]

    print(f"Compiling WandererMCM.psc...")
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout)
    if result.stderr:
        print(result.stderr, file=sys.stderr)

    pex_path = os.path.join(OUTPUT_DIR, "WandererMCM.pex")
    if result.returncode != 0 or not os.path.exists(pex_path):
        print("Compilation failed!", file=sys.stderr)
        sys.exit(1)

    size = os.path.getsize(pex_path)
    print(f"Generated {pex_path} ({size} bytes)")


if __name__ == "__main__":
    main()
