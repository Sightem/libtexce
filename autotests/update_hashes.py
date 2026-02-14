#!/usr/bin/env python3
"""
update_hashes.py - Update baseline CRCs in casegen/cases.c

Usage:
    ./update_hashes.py [--dry-run] [suite[/case] ...]

Examples:
    ./update_hashes.py                    # Update all generated cases
    ./update_hashes.py basic complex      # Update all cases in suites
    ./update_hashes.py basic/fraction_ab  # Update a specific case
    ./update_hashes.py --dry-run          # Show changes without writing

Requires AUTOTESTER_ROM environment variable to be set.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
DEFAULT_GENERATED_DIR = SCRIPT_DIR / "generated"
DEFAULT_CASES_FILE = SCRIPT_DIR / "casegen" / "cases.c"

def get_autotester_rom():
    """Get ROM path from environment."""
    rom = os.environ.get("AUTOTESTER_ROM")
    if not rom:
        print("Error: AUTOTESTER_ROM environment variable not set", file=sys.stderr)
        sys.exit(1)
    if not Path(rom).exists():
        print(f"Error: ROM file not found: {rom}", file=sys.stderr)
        sys.exit(1)
    return rom

def resolve_autotester() -> str:
    """Resolve autotester executable from env/path."""
    env_autotester = os.environ.get("AUTOTESTER")
    if env_autotester:
        return env_autotester

    found = shutil.which("autotester")
    if found:
        return found

    found = shutil.which("cemu-autotester")
    if found:
        return found

    cemu_path = os.environ.get("CEMU_PATH")
    if cemu_path:
        candidate = Path(cemu_path) / "tests" / "autotester" / "autotester"
        if candidate.exists():
            return str(candidate)

    print("Error: autotester executable not found.", file=sys.stderr)
    print("Set AUTOTESTER=/path/to/autotester or install 'autotester'/'cemu-autotester' in PATH.", file=sys.stderr)
    sys.exit(1)

def run_autotester(case_dir: Path, rom: str, autotester: str) -> str | None:
    """Run autotester and extract actual CRC for hash #1."""
    json_path = case_dir / "autotest.json"
    if not json_path.exists():
        print(f"  Skipping {case_dir}: no autotest.json", file=sys.stderr)
        return None

    case_name = case_dir.name
    case_bin = case_dir / "bin" / f"{case_name}.8xp"
    if not case_bin.exists():
        print("  Missing built program. Build generated cases before updating hashes.")
        print(f"  Expected: {case_bin}")
        return None

    result = subprocess.run(
        [autotester, "-d", str(json_path)],
        env={**os.environ, "AUTOTESTER_ROM": rom},
        capture_output=True,
        text=True,
    )

    crc = None
    matched = False
    for line in result.stdout.splitlines() + result.stderr.splitlines():
        match = re.search(r'Hash #1.*\(got ([0-9A-Fa-f]{1,8})\)', line)
        if match:
            crc = match.group(1).upper().zfill(8)  # Pad to 8 chars
            break
        match = re.search(r'Hash #1 had a matching CRC', line)
        if match:
            matched = True

    if crc:
        return crc
    if matched:
        print("  Hash #1 already matches expected CRC")
        return None

    if result.returncode != 0:
        print(f"  Autotester failed (exit {result.returncode}). Output:")
    else:
        print("  Autotester produced no CRC output. Output:")

    combined = (result.stdout + "\n" + result.stderr).strip()
    if combined:
        print("\n".join(f"  {line}" for line in combined.splitlines()))

    return None

def iter_generated_cases(generated_dir: Path):
    for suite_dir in sorted(generated_dir.iterdir()):
        if not suite_dir.is_dir():
            continue
        for case_dir in sorted(suite_dir.iterdir()):
            if not case_dir.is_dir():
                continue
            if (case_dir / "autotest.json").exists():
                yield suite_dir.name, case_dir.name, case_dir

def apply_case_updates(cases_path: Path, updates: dict[str, dict[str, str]], dry_run: bool) -> int:
    """Update expected_crc entries in casegen/cases.c. Returns count of updates."""
    text = cases_path.read_text()
    total_updates = 0

    for suite, suite_updates in updates.items():
        block_re = re.compile(
            rf'(static\s+const\s+TestCase\s+g_{re.escape(suite)}_cases\[\]\s*=\s*\{{)(.*?)(\}};)',
            re.DOTALL,
        )
        block_match = block_re.search(text)
        if not block_match:
            print(f"Warning: suite '{suite}' not found in {cases_path.name}", file=sys.stderr)
            continue

        block = block_match.group(2)
        block_updates = 0

        for case_name, crc in suite_updates.items():
            case_re = re.compile(
                rf'(\{{\s*"{re.escape(case_name)}"\s*,\s*[^,]*,\s*"(?:\\.|[^"\\])*"\s*,\s*)(NULL|"[0-9A-Fa-f]+")\s*(,\s*\d+\s*,\s*\d+\s*\}})',
                re.DOTALL,
            )
            case_match = case_re.search(block)
            if not case_match:
                print(f"Warning: case '{suite}/{case_name}' not found in cases.c", file=sys.stderr)
                continue

            old_crc = case_match.group(2)
            new_crc = f"\"{crc}\""
            if old_crc == new_crc:
                continue

            block = case_re.sub(rf'\1{new_crc}\3', block, count=1)
            block_updates += 1
            print(f"  [{suite}/{case_name}] {old_crc} -> {new_crc}")

        if block_updates > 0:
            total_updates += block_updates
            text = text[: block_match.start(2)] + block + text[block_match.end(2) :]

    if total_updates > 0 and not dry_run:
        cases_path.write_text(text)

    return total_updates

def main():
    parser = argparse.ArgumentParser(description="Update baseline CRCs in casegen/cases.c")
    parser.add_argument("cases", nargs="*", help="Suite or suite/case to update")
    parser.add_argument("--generated-dir", default=str(DEFAULT_GENERATED_DIR), help="Path to generated cases")
    parser.add_argument("--cases-file", default=str(DEFAULT_CASES_FILE), help="Path to casegen/cases.c")
    parser.add_argument("--dry-run", "-n", action="store_true", help="Show changes without writing")
    args = parser.parse_args()

    rom = get_autotester_rom()
    autotester = resolve_autotester()
    generated_dir = Path(args.generated_dir)
    cases_path = Path(args.cases_file)

    if not generated_dir.is_dir():
        print(f"Error: generated dir not found: {generated_dir}", file=sys.stderr)
        sys.exit(1)
    if not cases_path.exists():
        print(f"Error: cases file not found: {cases_path}", file=sys.stderr)
        sys.exit(1)

    filters = set(args.cases)
    updates: dict[str, dict[str, str]] = {}

    for suite, case_name, case_dir in iter_generated_cases(generated_dir):
        if filters:
            if f"{suite}/{case_name}" not in filters and suite not in filters:
                continue

        print(f"Checking {suite}/{case_name}...")
        crc = run_autotester(case_dir, rom, autotester)
        if not crc:
            print("  No CRC data captured")
            continue

        updates.setdefault(suite, {})[case_name] = crc

    print()
    total_updates = apply_case_updates(cases_path, updates, args.dry_run)
    if args.dry_run:
        print(f"Dry run: {total_updates} case(s) would be updated in {cases_path.name}")
    else:
        print(f"Updated {total_updates} case(s) in {cases_path.name}")

if __name__ == "__main__":
    main()
