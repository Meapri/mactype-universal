#!/usr/bin/env bash
set -euo pipefail

TAG="${1:-v1.2025.6.9}"
REPO="snowie2000/mactype"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
OUT_DIR="${ROOT_DIR}/installer/upstream"
TMP_DIR="${TMPDIR:-/tmp}/mactype_upstream"

mkdir -p "${OUT_DIR}" "${TMP_DIR}"

API_URL="https://api.github.com/repos/${REPO}/releases/tags/${TAG}"
JSON_FILE="${TMP_DIR}/release.json"
curl -sSL "${API_URL}" -o "${JSON_FILE}"

URLS=$(python3 - <<'PY'
import json,sys
data=json.load(open(sys.argv[1]))
assets=data.get('assets',[])
for a in assets:
    print(a.get('browser_download_url',''))
PY
"${JSON_FILE}")

ASSET_URL=$(printf "%s\n" "$URLS" | grep -iE '\\.zip$' | head -n1 || true)
if [ -z "${ASSET_URL}" ]; then
  ASSET_URL=$(printf "%s\n" "$URLS" | grep -iE '\\.(exe|msi)$' | head -n1 || true)
fi

if [ -z "${ASSET_URL}" ]; then
  echo "No downloadable asset found for tag ${TAG}" >&2
  exit 1
fi

ASSET_FILE="${TMP_DIR}/$(basename "${ASSET_URL}")"
echo "Downloading: ${ASSET_URL} -> ${ASSET_FILE}"
curl -L --fail -o "${ASSET_FILE}" "${ASSET_URL}"

if [[ "${ASSET_FILE}" == *.zip ]]; then
  echo "Extracting ZIP to ${OUT_DIR}"
  unzip -o "${ASSET_FILE}" -d "${OUT_DIR}" >/dev/null
else
  echo "Extracting installer with 7z to ${OUT_DIR}"
  if ! command -v 7z >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
      brew install -q p7zip
    else
      echo "7z not found and Homebrew unavailable" >&2
      exit 1
    fi
  fi
  7z x -y -o"${OUT_DIR}" "${ASSET_FILE}" >/dev/null
fi

CFG_DIR="${OUT_DIR}/config"
mkdir -p "${CFG_DIR}"
find "${OUT_DIR}" -type f \( -iname '*.ini' -o -iname '*.iss' \) -print0 | while IFS= read -r -d '' f; do
  echo "Collect: ${f#${OUT_DIR}/}"
  cp -f "$f" "${CFG_DIR}/"
done

echo "Done. Collected configs at ${CFG_DIR}"


