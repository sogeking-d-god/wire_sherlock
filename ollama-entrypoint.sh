#!/bin/sh
# Boot Ollama and ensure the requested model is present on the persistent volume.
# Idempotent: `ollama pull` is a no-op when the model is already cached locally.

set -e

MODEL="${OLLAMA_MODEL:-phi3:mini}"

# Start the daemon in the background; we use the same container as the server.
ollama serve &
SERVE_PID=$!

# Wait for the daemon's HTTP API to come up.
echo "[entrypoint] waiting for ollama daemon..."
until ollama list >/dev/null 2>&1; do
    sleep 1
done
echo "[entrypoint] daemon up — ensuring model '$MODEL' is available"

# Pull only if missing (cheap check first to avoid network calls).
if ! ollama list | awk 'NR>1 {print $1}' | grep -Fxq "$MODEL"; then
    echo "[entrypoint] pulling $MODEL ..."
    ollama pull "$MODEL"
else
    echo "[entrypoint] $MODEL already cached"
fi

echo "[entrypoint] ready"
wait "$SERVE_PID"
