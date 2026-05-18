import os
import bcrypt
from datetime import datetime, timezone, timedelta
from jose import jwt, JWTError  # noqa: F401 — JWTError re-exported for callers

# Load from environment; the default is a dev-only placeholder.
SECRET_KEY: str = os.environ.get("JWT_SECRET_KEY", "CHANGE_ME_IN_PRODUCTION")
ALGORITHM = "HS256"
TOKEN_EXPIRE_HOURS = 8


def hash_password(plain: str) -> str:
    return bcrypt.hashpw(plain.encode("utf-8"), bcrypt.gensalt()).decode("utf-8")


def verify_password(plain: str, hashed: str) -> bool:
    return bcrypt.checkpw(plain.encode("utf-8"), hashed.encode("utf-8"))


def create_access_token(user_id: str) -> str:
    expire = datetime.now(timezone.utc) + timedelta(hours=TOKEN_EXPIRE_HOURS)
    return jwt.encode({"sub": user_id, "exp": expire}, SECRET_KEY, algorithm=ALGORITHM)


def decode_access_token(token: str) -> str:
    """Validates JWT and returns the user_id string. Raises JWTError on failure."""
    payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
    user_id: str | None = payload.get("sub")
    if not user_id:
        raise JWTError("Missing sub claim")
    return user_id
