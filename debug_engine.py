import sys
import os

# הוספת התיקייה הנוכחית ל-Path כדי שיוכל למצוא את api.python
sys.path.append(os.getcwd())

try:
    from api.python.session_wrapper import WireSherlockSession
    print("Successfully imported WireSherlockSession")
except ImportError as e:
    print(f"Import Error: {e}")
    sys.exit(1)

def run_debug():
    pcap_path = "/home/ronen/wire_sherlock/pcap_files/regular_pcap_file.pcap"

    if not os.path.exists(pcap_path):
        print(f"File not found at: {pcap_path}")
        return

    print(f"--- Starting Debug Session for: {pcap_path} ---")

    try:
        # יצירת המופע - כאן בדרך כלל קורה ה-pcap_open_offline ב-C
        session = WireSherlockSession(pcap_path)
        print("Session object created successfully.")

        # הרצת האנליזה - כאן כנראה נזרקת שגיאת ה-Socket
        print("Running analysis...")
        result = session.run_analysis()

        print("--- Analysis Complete! ---")
        if result and hasattr(result, 'global_ipv4_stats'):
            print(f"Found {len(result.global_ipv4_stats)} IP nodes.")

    except Exception as e:
        print(f"\n[CRITICAL ERROR] Caught in Python: {e}")
        # אם יש לך גישה ל-errno או לפרטי שגיאה נוספים מה-Wrapper, כדאי להדפיס אותם כאן

if __name__ == "__main__":
    run_debug()