-- הפעלת הרחבה ליצירת UUID באופן אוטומטי (מובנה ב-PostgreSQL 13+)
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ==========================================
-- 1. טבלת משתמשים (Users)
-- ==========================================
CREATE TABLE users (
    user_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    is_online BOOLEAN NOT NULL DEFAULT FALSE,
    current_session_id UUID -- יקושר באלטר בהמשך הקובץ
);

-- ==========================================
-- 2. טבלת תחקירים (Analysis_Sessions)
-- ==========================================
CREATE TABLE analysis_sessions (
    session_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL,
    pcap_file_name VARCHAR(255) NOT NULL,
    pcap_file_path VARCHAR(512) NOT NULL UNIQUE,
    binary_dump_path VARCHAR(512) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_opened_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    engine_pid INTEGER,
    ipc_path VARCHAR(255),
    status VARCHAR(50) NOT NULL DEFAULT 'initializing',

    -- אילוצים קשיחים
    CONSTRAINT fk_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    CONSTRAINT chk_status CHECK (status IN ('initializing', 'running', 'stopped', 'error'))
);

-- הוספת המפתח הזר המעגלי לטבלת משתמשים
ALTER TABLE users
ADD CONSTRAINT fk_current_session
FOREIGN KEY (current_session_id) REFERENCES analysis_sessions(session_id) ON DELETE SET NULL;

-- ==========================================
-- 3. טבלת תובנות וסטטיסטיקות (Session_Insights)
-- ==========================================
CREATE TABLE session_insights (
    insight_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id UUID NOT NULL UNIQUE, -- אילוץ UNIQUE מבטיח קשר של 1:1
    time_bins_data JSONB,
    anomalies_data JSONB,
    dpi_alerts JSONB,

    CONSTRAINT fk_session FOREIGN KEY (session_id) REFERENCES analysis_sessions(session_id) ON DELETE CASCADE
);

-- יצירת אינדקסי GIN לחיפוש מהיר בתוך אובייקטי ה-JSONB האנליטיים
CREATE INDEX idx_insights_anomalies ON session_insights USING gin (anomalies_data);
CREATE INDEX idx_insights_dpi ON session_insights USING gin (dpi_alerts);

-- ==========================================
-- 4. טבלת היסטוריית צ'אט (LLM_Chat_History)
-- ==========================================
CREATE TABLE llm_chat_history (
    message_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id UUID NOT NULL,
    sequence_number INTEGER NOT NULL,
    sender_role VARCHAR(10) NOT NULL,
    message_content TEXT NOT NULL,
    timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT fk_chat_session FOREIGN KEY (session_id) REFERENCES analysis_sessions(session_id) ON DELETE CASCADE,
    CONSTRAINT chk_sender_role CHECK (sender_role IN ('user', 'agent'))
);

CREATE OR REPLACE VIEW v_recent_investigations AS
SELECT
    s.session_id,
    s.user_id,
    u.username,
    s.pcap_file_name,
    s.status,
    s.created_at,
    s.last_opened_at,
    -- ספירת ההתראות מתוך ה-JSONB במידה וקיים מערך
    CASE
        WHEN i.dpi_alerts IS NOT NULL AND jsonb_typeof(i.dpi_alerts) = 'array'
        THEN jsonb_array_length(i.dpi_alerts)
        ELSE 0
    END AS total_alerts,
    -- דגל אינדיקציה מהיר לפרונטאנד
    CASE
        WHEN i.dpi_alerts IS NOT NULL AND jsonb_typeof(i.dpi_alerts) = 'array' AND jsonb_array_length(i.dpi_alerts) > 0
        THEN TRUE
        ELSE FALSE
    END AS has_attacks
FROM analysis_sessions s
JOIN users u ON s.user_id = u.user_id
LEFT JOIN session_insights i ON s.session_id = i.session_id;

-- פונקציית הטריגר
CREATE OR REPLACE FUNCTION fn_cleanup_session_files()
RETURNS TRIGGER AS $$
BEGIN
    -- שליחת הודעה אסינכרונית לערוץ פנימי בשם pcap_cleanup_channel
    PERFORM pg_notify(
        'pcap_cleanup_channel',
        json_build_object(
            'event', 'DELETE',
            'pcap_file_path', OLD.pcap_file_path,
            'binary_dump_path', OLD.binary_dump_path
        )::text
    );
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_cleanup_session_files
BEFORE DELETE ON analysis_sessions
FOR EACH ROW
EXECUTE FUNCTION fn_cleanup_session_files();