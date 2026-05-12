from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from backend.routers import topology_router, pcap_router, metrics_router

app = FastAPI(title="WireSherlock API", version="1.0.0")

origins = [
    "http://localhost:5173",  # frontend port
    "http://127.0.0.1:5173",
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],)

# Register routers
app.include_router(topology_router.router)
app.include_router(pcap_router.router)
app.include_router(metrics_router.router)

@app.get("/")
async def root():
    return {"message": "WireSherlock Backend is Up and Running!"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)