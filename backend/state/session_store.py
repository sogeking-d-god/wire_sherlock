class SessionStore:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(SessionStore, cls).__new__(cls)
            cls._instance.current_analysis = None
            cls._instance.topology = None
            cls._instance.metrics = {} # BINS
        return cls._instance

store = SessionStore()