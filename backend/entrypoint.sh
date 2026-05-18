#!/bin/bash
set -euo pipefail

ENGINE_DIR="/app/api/engine"
MANIFEST="$ENGINE_DIR/.build_manifest"

echo "[WireSherlock] Checking C engine sources..."
cd /app

# Content-based fingerprint of every .c/.h file across the whole source tree.
# sort guarantees a stable ordering so identical trees produce identical output.
CURRENT=$(find . \( -name "*.c" -o -name "*.h" \) | sort | xargs md5sum 2>/dev/null || true)

if [ ! -f "$MANIFEST" ]; then
    echo "[WireSherlock] No prior build cache — full build."
    cd "$ENGINE_DIR" && make

elif [ "$CURRENT" = "$(cat "$MANIFEST")" ]; then
    echo "[WireSherlock] No source changes — skipping compilation."

else
    # Diff the old and new manifests; lines prefixed '>' are new/changed files.
    CHANGED=$(diff <(cat "$MANIFEST") <(echo "$CURRENT") | grep '^>' | awk '{print $2}')

    H_FILES=$(echo "$CHANGED" | grep '\.h$' || true)
    C_FILES=$(echo "$CHANGED" | grep '\.c$' || true)

    if [ -n "$H_FILES" ]; then
        # The Makefile has no -MMD dependency tracking, so a changed header
        # could affect any translation unit. Safe option: clean rebuild.
        H_COUNT=$(echo "$H_FILES" | wc -l | tr -d ' ')
        echo "[WireSherlock] $H_COUNT header(s) changed — clean rebuild:"
        echo "$H_FILES" | sed 's/^/  /'
        cd "$ENGINE_DIR" && make clean && make

    else
        # Only .c files changed: touch them so their mtime is definitively
        # newer than the corresponding .o inside the container, then let
        # make do a true incremental build.
        C_COUNT=$(echo "$C_FILES" | wc -l | tr -d ' ')
        echo "[WireSherlock] $C_COUNT source(s) changed — incremental build:"
        echo "$C_FILES" | sed 's/^/  /'
        echo "$C_FILES" | xargs -r touch
        cd "$ENGINE_DIR" && make
    fi

    echo "[WireSherlock] Build complete."
fi

# Persist the fingerprint back to the volume so the next startup can diff against it.
echo "$CURRENT" > "$MANIFEST"

echo "[WireSherlock] C engine ready."
cd /app
exec "$@"
