from backend import session_manager
from api.python.session_wrapper import WireSherlockSession
from fastapi import APIRouter

router = APIRouter(prefix="/api/pcap", tags=["PCAP"])

@router.post("/initialize_default")
async def initialize_default_session():
    test_pcap = "/home/ronen/wire_sherlock/pcap_files/regular_pcap_file.pcap"

    # 1. Create the session object
    session = WireSherlockSession(test_pcap)
    session.start()
    session.run_analysis() # Pre-fill the bins in C

    # 2. SAVE it to the manager so others can see it
    session_manager.set_session(session)

    return {"status": "session_initialized", "msg": "Session is now global"}