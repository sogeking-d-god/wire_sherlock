import os
import shutil

from fastapi import APIRouter, Depends, File, HTTPException, UploadFile

from api.config import DEMO_PATH, UPLOAD_DIR
from backend.dependencies import get_current_user
from backend.sessions.session_manager import manager

from .endpoints import PCAPEndpoints

router = APIRouter(prefix=PCAPEndpoints.PREFIX, tags=["PCAP"])


@router.post(PCAPEndpoints.SELECT)
async def select_file(file_id: str, user_id: str = Depends(get_current_user)):
    safe_file_id = os.path.basename(file_id)
    if file_id == "demo-pcap":
        target_path = DEMO_PATH
    else:
        target_path = os.path.join(UPLOAD_DIR, safe_file_id)

    if not os.path.exists(target_path):
        raise HTTPException(status_code=404, detail="File not found")

    try:
        await manager.create_or_replace(user_id, target_path, file_id)
        return {"status": "success", "active_file": file_id}
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Lifecycle Error: {str(e)}")


@router.get(PCAPEndpoints.ACTIVE_SESSION)
async def get_active_session(user_id: str = Depends(get_current_user)):
    label = manager.get_label(user_id)
    if not label:
        return {"active": False}
    return {"active": True, "filename": label, "file_id": label}


@router.post(PCAPEndpoints.UPLOAD)
async def upload_pcap(
    file: UploadFile = File(...),
    user_id: str = Depends(get_current_user),
):
    if not file.filename.endswith((".pcap", ".pcapng")):
        raise HTTPException(status_code=400, detail="Only PCAP files are supported.")

    safe_name = os.path.basename(file.filename)
    file_path = os.path.join(UPLOAD_DIR, safe_name)

    try:
        with open(file_path, "wb") as buffer:
            shutil.copyfileobj(file.file, buffer)

        return {
            "status": "success",
            "filename": safe_name,
            "message": f"File '{safe_name}' uploaded successfully.",
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Upload failed: {str(e)}")
    finally:
        file.file.close()


@router.get(PCAPEndpoints.FILES)
async def list_files(user_id: str = Depends(get_current_user)):
    try:
        files = [f for f in os.listdir(UPLOAD_DIR) if f.endswith((".pcap", ".pcapng"))]
        all_files = [{"id": "demo-pcap", "name": "demo-pcap (Sample)"}]
        for f in files:
            all_files.append({"id": f, "name": f})
        return all_files
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to list files: {str(e)}")
