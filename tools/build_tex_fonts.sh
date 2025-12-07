#!/usr/bin/env bash
# build_tex_fonts.sh - Convert TeX font sources to 8xv binaries using convfont/convbin.
# Usage:
#   tools/build_tex_fonts.sh [--outdir PATH] [--dry-run] [--force] [--help]
#
# Default behavior follows the commands the user provided.
# Creates assets/TeXFonts.8xv and assets/TeXScrpt.8xv

set -euo pipefail
IFS=$'\n\t'

# Default settings
OUTDIR="assets"
DRY_RUN=false
FORCE=false
QUIET=false
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Files and names
MAIN_TXT="${ROOT_DIR}/tex_main.txt"
SCRIPT_TXT="${ROOT_DIR}/tex_script.txt"
MAIN_BIN="tex_main.bin"
SCRIPT_BIN="tex_script.bin"
MAIN_NAME="TeXFonts"
SCRIPT_NAME="TeXScrpt"

usage() {
	cat <<EOF
Usage: $(basename "$0") [options]

Options:
	--outdir PATH    Output directory for created files (default: assets)
	--dry-run        Display commands that would be executed without running them
	--force          Overwrite existing output files
	--quiet          Minimize messages
	-h, --help       Show this help message

This script runs the following steps:
	1) Run convfont to produce font pack .bin files from the .txt definitions
	2) Run convbin to convert .bin files to 8xv
	3) Place results in the specified output directory

Example:
	tools/build_tex_fonts.sh --outdir assets
EOF
}

log() {
	local level="$1"; shift
	if [ "$QUIET" = false ]; then
		printf "[%s] %s\n" "$level" "$*" >&2
	fi
}

run_cmd() {
	if [ "$DRY_RUN" = true ]; then
		printf "DRY RUN: %s\n" "$*"
	else
		"$@"
	fi
}

check_deps() {
	for tool in convfont convbin; do
		if ! command -v "$tool" >/dev/null 2>&1; then
			echo "Error: Required tool '$tool' not found in PATH." >&2
			echo "Please install $tool or adjust PATH and retry." >&2
			exit 1
		fi
	done
}

# Parse arguments
while [[ $# -gt 0 ]]; do
	case "$1" in
		--outdir)
			OUTDIR="$2"; shift 2;;
		--dry-run)
			DRY_RUN=true; shift;;
		--force)
			FORCE=true; shift;;
		--quiet)
			QUIET=true; shift;;
		-h|--help)
			usage; exit 0;;
		*)
			echo "Unknown argument: $1"; usage; exit 1;;
	esac
done

# Sanity checks
if [ ! -f "$MAIN_TXT" ]; then
	echo "Error: main font definition not found: $MAIN_TXT" >&2; exit 1
fi
if [ ! -f "$SCRIPT_TXT" ]; then
	echo "Error: script font definition not found: $SCRIPT_TXT" >&2; exit 1
fi

mkdir -p "$OUTDIR"

MAIN_OUT_PATH="$OUTDIR/${MAIN_NAME}.8xv"
SCRIPT_OUT_PATH="$OUTDIR/${SCRIPT_NAME}.8xv"

if [ "$DRY_RUN" = false ] && [ "$FORCE" = false ]; then
	if [ -f "$MAIN_OUT_PATH" ]; then
		echo "Error: $MAIN_OUT_PATH already exists (use --force to overwrite)." >&2
		exit 1
	fi
	if [ -f "$SCRIPT_OUT_PATH" ]; then
		echo "Error: $SCRIPT_OUT_PATH already exists (use --force to overwrite)." >&2
		exit 1
	fi
fi

if [ "$DRY_RUN" = false ]; then
	check_deps
fi

# Build main font pack
log INFO "Building main TeX font pack (output: $MAIN_OUT_PATH)"
MAIN_CONV_CMD=(convfont -o fontpack -N "TeX Main" -P "Win-1252" -D "Main 16px Math Font" -t "$MAIN_TXT" -a 0 -w normal -s sans-serif "$MAIN_BIN")
if [ "$DRY_RUN" = true ]; then
	echo "${MAIN_CONV_CMD[*]}"
else
	printf "Running: %s\n" "${MAIN_CONV_CMD[*]}"
	"${MAIN_CONV_CMD[@]}"
fi

# Convert bin to 8xv
MAIN_CONV_BIN_CMD=(convbin -r -k 8xv -n "$MAIN_NAME" -i "$MAIN_BIN" -o "$MAIN_OUT_PATH")
run_cmd "${MAIN_CONV_BIN_CMD[@]}"

# Build script font pack
log INFO "Building TeX Script font pack (output: $SCRIPT_OUT_PATH)"
SCRIPT_CONV_CMD=(convfont -o fontpack -N "TeX Script" -P "Win-1252" -D "Script 12px Math Font" -t "$SCRIPT_TXT" -a 0 -w normal "$SCRIPT_BIN")
run_cmd "${SCRIPT_CONV_CMD[@]}"

# Convert bin to 8xv
SCRIPT_CONV_BIN_CMD=(convbin -r -k 8xv -n "$SCRIPT_NAME" -i "$SCRIPT_BIN" -o "$SCRIPT_OUT_PATH")
run_cmd "${SCRIPT_CONV_BIN_CMD[@]}"

# Cleanup (optional): remove intermediate .bin files if we successfully created outputs
if [ "$DRY_RUN" = false ]; then
	if [ -f "$MAIN_OUT_PATH" ] && [ -f "$SCRIPT_OUT_PATH" ]; then
		log INFO "Font conversion finished:"
		log INFO "  $MAIN_OUT_PATH"
		log INFO "  $SCRIPT_OUT_PATH"
		# Keep .bin files by default; remove them only if FORCE is provided
		if [ "$FORCE" = true ]; then
			log INFO "Cleaning up intermediate files"
			rm -f "$MAIN_BIN" "$SCRIPT_BIN"
		fi
	else
		echo "Error: conversion did not produce expected files." >&2
		exit 1
	fi
fi

exit 0

