#!/usr/bin/env bash
# relightmap.sh for SDHLT v1.3.1 - recompiles light in any map
# ------------------------------------------------------------
# Keep tool binaries as shipped in Win64/, Win32/ or Linux/

# Optional: a .wa_ file next to the input map (compile-time WAD)
# containing all used map textures, sed for bounced light,
# texlights and transparency. Not required for maps with all
# textures embedded (i.e. -nowadtextures)
#
# Usage: ./relightmap.sh [-t tool-binary] <map.bsp> [rad params...]
#   -t  use a specific RAD binary if none found next to this script
#   -h  prints this help
#
# Example: ./relightmap.sh -t Linux/sdHLRAD c1a0.bsp -extra -ao -aostats -smooth 240 -smooth2 240 -low
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
help() { sed -n '2,/^[^#]/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'; } #Self-adjusting range

pick_rad() {
  local c out
  for c in "$DIR/Win64/sdHLRAD_x64.exe" "$DIR/Win32/sdHLRAD.exe" "$DIR/Linux/sdHLRAD"; do
    [[ -x "$c" ]] || continue
    out="$("$c" 2>&1 || true)"
    [[ "$out" == *"sdHLRAD v"* ]] && { printf '%s\n' "$c"; return 0; }
  done
  return 1
}

TOOL=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--tool) TOOL="$2"; shift 2 ;;
    -h|--help) help; exit 0 ;;
    *)         break ;;
  esac
done

[[ -n "$TOOL" ]] || TOOL="$(pick_rad)" || {
  echo "no usable RAD build found under $DIR (Win64/, Win32/, Linux/)" >&2
  echo "pass one explicitly: $0 -t /path/to/binary <map.bsp>" >&2
  exit 1
}
[[ -x "$TOOL" ]] || { echo "RAD binary not found: $TOOL" >&2; exit 1; }
[[ $# -ge 1 ]]  || { help; exit 1; }

MAP="$1"; shift

[[ -f "$MAP" ]] || { [[ -f "$MAP.bsp" ]] && MAP="$MAP.bsp" || { echo "not found: $MAP[.bsp]" >&2; exit 1; }; }
OUT="${MAP%.bsp}_relight.bsp"
OUTWAD="${MAP%.bsp}_relight.wa_"

cp -f -- "$MAP" "$OUT"
WAD="${MAP%.bsp}.wa_"

if [[ -f "$WAD" ]]; then
  cp -f -- "$WAD" "$OUTWAD"
else
  echo "warning: no $WAD found, assuming textures are embedded" >&2
fi

echo "-> $TOOL \"$OUT\" $*"

if "$TOOL" "$OUT" "$@"; then
  rm -f -- "$OUTWAD"
  echo "RAD complete: $OUT"
else
  status=$?
  rm -f -- "$OUT" "$OUTWAD"
  echo "RAD failed (exit $status)" >&2
  exit "$status"
fi
