from typing import Optional
from api.python.session_wrapper import WireSherlockSession

class SessionManager:
    def __init__(self):
        self._current_session: Optional[WireSherlockSession] = None
        self._active_filename: Optional[str] = None

    def set_session(self, pcap_path: str, filename: str):
        """
        The function clposes any existing session and starts a new one with the given PCAP path.
         It also runs the analysis immediately to prepare the data for the frontend.
        """
        # 1. Clean up old session if exists
        if self._current_session:
            print(f"[Manager] Stopping old session for: {self._active_filename}")
            try:
                self._current_session.stop() # closes the C process and socket
            except Exception as e:
                print(f"[Manager] Error stopping session: {e}")

        # 2. Start new session
        print(f"[Manager] Starting new session for: {filename}")
        new_session = WireSherlockSession(pcap_path)

        # 3. Initialization
        new_session.start()
        new_session.run_analysis()

        # 4. Update state
        self._current_session = new_session
        self._active_filename = filename

    def get_session(self) -> Optional[WireSherlockSession]:
        return self._current_session

    def get_active_filename(self) -> Optional[str]:
        return self._active_filename

# Global instance of the manager to be used across the backend
manager = SessionManager()