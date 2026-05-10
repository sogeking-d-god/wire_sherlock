import os
from fastapi import APIRouter, HTTPException, File, UploadFile
import shutil
from backend import session_manager
from api.config import DEMO_PATH, UPLOAD_DIR
router = APIRouter(prefix="/api/pcap", tags=["PCAP Management"])

@router.post("/select/{file_id}")
async def select_file(file_id: str):
    target_path = ""
    if file_id == "demo-pcap":
        target_path = DEMO_PATH
    else:
        target_path = os.path.join(UPLOAD_DIR, file_id)

    if not os.path.exists(target_path):
        raise HTTPException(status_code=404, detail="File not found")

    try:
        session_manager.manager.set_session(target_path, file_id)
        return {"status": "success", "active_file": file_id}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Lifecycle Error: {str(e)}")

@router.get("/active-session")
async def get_active_session():
    filename = session_manager.manager.get_active_filename()
    if not filename:
        return {"active": False}

    return {"active": True, "filename": filename}

@router.post("/upload")
async def upload_pcap(file: UploadFile = File(...)):
    if not file.filename.endswith(('.pcap', '.pcapng')):
        raise HTTPException(status_code=400, detail="Only PCAP files are supported.")

    file_path = os.path.join(UPLOAD_DIR, file.filename)

    try:
        with open(file_path, "wb") as buffer:
            shutil.copyfileobj(file.file, buffer)

        return {
            "status": "success",
            "filename": file.filename,
            "message": f"File '{file.filename}' uploaded successfully."
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Upload failed: {str(e)}")
    finally:
        file.file.close()

@router.get("/files")
async def list_files():
    """Returns a list of all available PCAP files in the uploads directory."""
    try:
        files = [f for f in os.listdir(UPLOAD_DIR) if f.endswith(('.pcap', '.pcapng'))]

        all_files = [{"id": "demo-pcap", "name": "demo-pcap (Sample)"}]

        for f in files:
            all_files.append({"id": f, "name": f})

        return all_files
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to list files: {str(e)}")