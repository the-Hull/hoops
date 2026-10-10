#!/usr/bin/env bash
# Build the watch app and pack the apps.unawatch.com upload zip.
#
# Usage: tools/package_store.sh [--allow-placeholder]
#
# Needs UNA_SDK (export UNA_SDK=$HOME/una-sdk) and the repo's .venv. Builds
# Software/Apps/Hoops-CMake, then assembles dist/pkg/ with
#   Hoops_<ver>.uapp, icon.png, app-manifest.json, previews/*.png
# (the manifest is store/app-manifest.json with "binary" and "appVersion"
# filled in and "minKernelVersion" stamped by the SDK resolver) and zips it to
# dist/Hoops-<ver>.zip. Refuses to run while APP_ID in CMakeLists.txt is still
# the local placeholder, unless --allow-placeholder is given (test packages).
set -euo pipefail

PLACEHOLDER_ID="13F039C6FE024E86"

allow_placeholder=0
for arg in "$@"; do
    case "$arg" in
        --allow-placeholder) allow_placeholder=1 ;;
        -h|--help) sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

cd "$(dirname "$0")/.."
ROOT="$PWD"
PY="$ROOT/.venv/bin/python3"
APP_DIR="$ROOT/Software/Apps/Hoops-CMake"
MANIFEST_SRC="$ROOT/store/app-manifest.json"
PREVIEWS="$ROOT/docs/store/previews"

: "${UNA_SDK:?UNA_SDK is not set (export UNA_SDK=\$HOME/una-sdk)}"
[[ -x "$PY" ]] || { echo "missing $PY (create the .venv first)" >&2; exit 1; }
KVER="$UNA_SDK/Utilities/Scripts/app_packer/min_kernel_version.py"
[[ -f "$KVER" ]] || { echo "missing $KVER" >&2; exit 1; }

app_id=$(sed -n 's/^set(APP_ID "\([0-9A-Fa-f]*\)").*/\1/p' "$APP_DIR/CMakeLists.txt")
[[ -n "$app_id" ]] || { echo "APP_ID not found in $APP_DIR/CMakeLists.txt" >&2; exit 1; }
if [[ "$app_id" == "$PLACEHOLDER_ID" && $allow_placeholder -eq 0 ]]; then
    echo "APP_ID is still the placeholder $PLACEHOLDER_ID: set the App ID from apps.unawatch.com" >&2
    echo "in CMakeLists.txt and store/app-manifest.json (or pass --allow-placeholder)" >&2
    exit 1
fi
manifest_id=$("$PY" -I -c 'import json,sys; print(json.load(open(sys.argv[1]))["id"])' "$MANIFEST_SRC")
if [[ "$manifest_id" != "$app_id" ]]; then
    echo "store/app-manifest.json id ($manifest_id) differs from APP_ID ($app_id)" >&2
    exit 1
fi

shopt -s nullglob
previews=("$PREVIEWS"/*.png)
(( ${#previews[@]} > 0 )) || { echo "no previews in $PREVIEWS" >&2; exit 1; }

# Build (old .uapp files go first so exactly one is left to pick up).
rm -f "$APP_DIR"/build/Hoops_*.uapp
(cd "$APP_DIR" &&
    cmake -S . -B build -DUNA_PYTHON_EXECUTABLE="$PY" &&
    cmake --build build -j)
uapps=("$APP_DIR"/build/Hoops_*.uapp)
(( ${#uapps[@]} == 1 )) || { echo "expected one build/Hoops_*.uapp, found ${#uapps[@]}" >&2; exit 1; }
uapp_name=$(basename "${uapps[0]}")
ver=${uapp_name#Hoops_}
ver=${ver%.uapp}

PKG="$ROOT/dist/pkg"
ZIP="$ROOT/dist/Hoops-$ver.zip"
rm -rf "$PKG" "$ZIP"
mkdir -p "$PKG/previews"
cp "${uapps[0]}" "$PKG/$uapp_name"
cp "$ROOT/store/icon.png" "$PKG/icon.png"
cp "${previews[@]}" "$PKG/previews/"
"$PY" -I -c '
import json, sys
src, dst, binary, ver = sys.argv[1:5]
with open(src) as f:
    m = json.load(f)
m["binary"] = binary
m["appVersion"] = ver
with open(dst, "w") as f:
    json.dump(m, f, indent=2)
    f.write("\n")
' "$MANIFEST_SRC" "$PKG/app-manifest.json" "$uapp_name" "$ver"

"$PY" -I "$KVER" --stamp "$PKG/app-manifest.json"
"$PY" -I "$KVER" --check "$PKG/app-manifest.json"

(cd "$PKG" && "$PY" -I -m zipfile -c "$ZIP" "$uapp_name" icon.png app-manifest.json previews)
echo "wrote ${ZIP#"$ROOT"/} ($uapp_name, icon.png, app-manifest.json, ${#previews[@]} previews)"
