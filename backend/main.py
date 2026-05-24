import asyncio
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from backend.routers import auth_router, metrics_router, pcap_router, topology_router
from backend.routers import anomalies_router, http_attacks_router
from backend.sessions import reaper
from backend.sessions.session_manager import manager


@asynccontextmanager
async def lifespan(app: FastAPI):
    reaper_task = asyncio.create_task(reaper.run(manager))
    app.state.reaper_task = reaper_task
    try:
        yield
    finally:
        reaper_task.cancel()
        try:
            await reaper_task
        except asyncio.CancelledError:
            pass
        await manager.terminate_all()


app = FastAPI(title="WireSherlock API", version="1.0.0", lifespan=lifespan)

# allow_credentials=True requires an explicit origin list — "*" is forbidden by browsers
# when cookies are involved.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://127.0.0.1:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(auth_router.router)
app.include_router(topology_router.router)
app.include_router(pcap_router.router)
app.include_router(metrics_router.router)
app.include_router(anomalies_router.router)
app.include_router(http_attacks_router.router)


@app.get("/")
async def root():
    return {"message": "WireSherlock Backend is Up and Running!"}


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
