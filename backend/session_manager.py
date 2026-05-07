from api.python.session_wrapper import WireSherlockSession

# This variable stays in the server's RAM
_active_session: WireSherlockSession = None

def set_session(session: WireSherlockSession):
    """Stores the active session in the central manager."""
    global _active_session
    _active_session = session

def get_session() -> WireSherlockSession:
    """Retrieves the currently active session for any router to use."""
    global _active_session
    return _active_session