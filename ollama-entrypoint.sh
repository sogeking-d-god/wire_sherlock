#!/bin/sh
# Boot Ollama and (best-effort) ensure the requested model is present.
#
# Why this script is more defensive than a one-liner `ollama pull`:
#   - Docker Desktop's embedded DNS resolver (127.0.0.11) is flaky on Windows
#     under heavy load. A 2 GB blob pull frequently succeeds, but the final
#     manifest-verify HEAD request to registry.ollama.ai hits an i/o timeout
#     and `ollama pull` exits non-zero. The blobs are on disk; only the tiny
#     manifest file is missing. Re-running the pull finishes the job in
#     seconds (the blob digests are content-addressed and already cached).
#   - We therefore retry the pull with backoff instead of giving up.
#   - Critically: even if all retries fail, we DO NOT kill the container.
#     `ollama serve` keeps running so the backend's warmup-retry loop has a
#     responsive daemon to talk to, and the user can `docker restart ollama`
#     once their network stabilizes — cached blobs make the retry near-instant.

MODEL="${OLLAMA_MODEL:-phi3:mini}"
MAX_PULL_ATTEMPTS="${OLLAMA_PULL_MAX_ATTEMPTS:-10}"
PULL_BACKOFF_S="${OLLAMA_PULL_BACKOFF_S:-15}"

# Start the daemon in the background; we use the same container as the server.
ollama serve &
SERVE_PID=$!

# Wait for the daemon's HTTP API to come up before pulling.
echo "[entrypoint] waiting for ollama daemon..."
until ollama list >/dev/null 2>&1; do
    sleep 1
done
echo "[entrypoint] daemon up — checking for model '$MODEL'"

# Cheap presence check first. `ollama list` shows only models with a complete
# manifest, so if it lists $MODEL we know the pull finished cleanly.
if ollama list | awk 'NR>1 {print $1}' | grep -Fxq "$MODEL"; then
    echo "[entrypoint] $MODEL already cached"
else
    echo "[entrypoint] pulling $MODEL (max $MAX_PULL_ATTEMPTS attempts, ${PULL_BACKOFF_S}s backoff)"
    attempt=1
    while [ "$attempt" -le "$MAX_PULL_ATTEMPTS" ]; do
        echo "[entrypoint] pull attempt $attempt/$MAX_PULL_ATTEMPTS..."
        if ollama pull "$MODEL"; then
            echo "[entrypoint] pull succeeded on attempt $attempt"
            break
        fi
        # Pull failed — likely DNS or transient registry issue. The cached
        # blobs survive, so the next attempt is much faster than the first.
        if [ "$attempt" -lt "$MAX_PULL_ATTEMPTS" ]; then
            echo "[entrypoint] pull attempt $attempt failed — retrying in ${PULL_BACKOFF_S}s"
            sleep "$PULL_BACKOFF_S"
        else
            echo "[entrypoint] pull gave up after $MAX_PULL_ATTEMPTS attempts; daemon stays up so backend can serve auth/UI"
        fi
        attempt=$((attempt + 1))
    done
fi

echo "[entrypoint] ready"
# Hand control back to the daemon. If `ollama serve` ever exits, the container
# exits with it — which is what we want (Docker will restart it per policy).
wait "$SERVE_PID"
