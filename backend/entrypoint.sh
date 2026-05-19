#!/bin/bash
set -euo pipefail

ENGINE_DIR="/app/api/engine"

echo "[WireSherlock] Forcing a clean engine build..."
cd "$ENGINE_DIR" && make clean && make
echo "[WireSherlock] C engine ready."
cd /app
exec "$@"
