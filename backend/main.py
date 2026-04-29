from fastapi import FastAPI, HTTPException, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware
from typing import Dict
import uuid

# ייבוא מהתיקייה החיצונית (בהנחה שהיא ב-PYTHONPATH או באותה רמה)
from API.session_manager import WireSherlockSession, MetricType
from processors.topology import TopologyBuilder

app = FastAPI(title="WireSherlock API")

# הגדרת CORS כדי שה-React (בפורט 3000 בדרך כלל) יוכל לדבר עם ה-API
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# "מחסן" לסשנים פעילים בתוך הזיכרון של השרת
sessions: Dict[str, WireSherlockSession] = {}

@app.post("/analyze/start")
async def start_analysis(pcap_path: str):
    """
    יוצר סשן חדש, מפעיל את מנוע ה-C ומתחיל ניתוח
    """
    if not pcap_path:
        raise HTTPException(status_code=400, detail="PCAP path is required")

    try:
        session = WireSherlockSession(pcap_path)
        session.start_engine()

        # הרצת הניתוח הראשוני (Parsing)
        response = session.send_command("cmd_start_analysis") # השתמש במחרוזת מה-ipc_config שלך

        if response.get('status') != 'status_success':
            session.close()
            raise HTTPException(status_code=500, detail="C Engine failed to analyze")

        # שמירת הסשן במחסן לפי ה-ID שלו
        sessions[session.session_id] = session

        return {
            "session_id": session.session_id,
            "summary": response.get('data')
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/analyze/{session_id}/topology")
async def get_topology(session_id: str):
    """
    מושך נתונים מה-C ומעביר אותם דרך ה-TopologyBuilder ל-GUI
    """
    session = sessions.get(session_id)
    if not session:
        raise HTTPException(status_code=404, detail="Session not found")

    # שליחת פקודה ל-C לקבלת כל הנתונים הגולמיים (נניח CMD_GET_FULL_STATS)
    raw_data = session.send_command("cmd_get_full_stats")

    if not raw_data or raw_data.get('status') != 'status_success':
         raise HTTPException(status_code=500, detail="Failed to fetch stats from engine")

    # שימוש ב-Processor שלך לבניית הגרף
    topology = TopologyBuilder.build(raw_data.get('data'))
    return topology

@app.delete("/analyze/{session_id}")
async def stop_analysis(session_id: str):
    """
    סגירת סשן ושחרור משאבים
    """
    session = sessions.pop(session_id, None)
    if session:
        session.close()
        return {"status": "closed"}
    raise HTTPException(status_code=404, detail="Session not found")