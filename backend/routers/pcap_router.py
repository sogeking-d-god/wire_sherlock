from fastapi import APIRouter, UploadFile, File, HTTPException
import shutil
import os
from uuid import uuid4

router = APIRouter(prefix="/api/pcap", tags=["PCAP Management"])

UPLOAD_DIR = "uploads"

# Check that the upload directory exists, if not create it
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)

@router.post("/upload")
async def upload_pcap(file: UploadFile = File(...)):
    # 1. Check if the uploaded file is a valid PCAP file based on its extension
    if not file.filename.endswith(('.pcap', '.pcapng')):
        raise HTTPException(status_code=400, detail="Invalid file type. Only .pcap and .pcapng are supported.")

    # 2. Create a unique ID for the file (to avoid name conflicts)
    file_id = str(uuid4())
    file_extension = os.path.splitext(file.filename)[1]
    saved_filename = f"{file_id}{file_extension}"
    file_path = os.path.join(UPLOAD_DIR, saved_filename)

    # 3. Save the file to disk
    try:
        with open(file_path, "wb") as buffer:
            shutil.copyfileobj(file.file, buffer)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to save file: {str(e)}")

    return {
        "status": "success",
        "file_id": file_id,
        "original_name": file.filename,
        "saved_path": file_path
    }