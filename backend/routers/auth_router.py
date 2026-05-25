from fastapi import APIRouter, Depends, HTTPException, Response, status
from sqlalchemy.ext.asyncio import AsyncSession
from pydantic import BaseModel, field_validator
from sqlalchemy.exc import IntegrityError

from backend.db.database import get_db
from backend.db import crud
from backend.dependencies import get_current_user
from backend.services.auth_service import verify_password, create_access_token, TOKEN_EXPIRE_HOURS
from backend.sessions.session_manager import manager as session_manager

router = APIRouter(prefix="/auth", tags=["Auth"])

_COOKIE = "access_token"
_COOKIE_MAX_AGE = TOKEN_EXPIRE_HOURS * 3600


class AuthRequest(BaseModel):
    username: str
    password: str

    @field_validator("username")
    @classmethod
    def username_valid(cls, v: str) -> str:
        v = v.strip()
        if not 3 <= len(v) <= 50:
            raise ValueError("Username must be 3–50 characters")
        return v

    @field_validator("password")
    @classmethod
    def password_valid(cls, v: str) -> str:
        if len(v) < 8:
            raise ValueError("Password must be at least 8 characters")
        return v


def _set_auth_cookie(response: Response, token: str) -> None:
    response.set_cookie(
        key=_COOKIE,
        value=token,
        httponly=True,       # JS cannot read this cookie — blocks XSS token theft
        samesite="lax",      # sent on top-level navigations, blocks cross-site POST CSRF
        secure=False,        # set True in production (requires HTTPS)
        max_age=_COOKIE_MAX_AGE,
        path="/",
    )


@router.post("/signup", status_code=status.HTTP_201_CREATED)
async def signup(
    body: AuthRequest,
    response: Response,
    db: AsyncSession = Depends(get_db),
):
    try:
        user = await crud.create_user(db, body.username, body.password)
    except IntegrityError:
        raise HTTPException(status_code=409, detail="Username already taken")

    token = create_access_token(str(user.user_id))
    _set_auth_cookie(response, token)
    return {"username": user.username}


@router.post("/login")
async def login(
    body: AuthRequest,
    response: Response,
    db: AsyncSession = Depends(get_db),
):
    user = await crud.get_user_by_username(db, body.username)

    # Constant-time path: always verify even on miss to prevent user-enumeration via timing
    dummy_hash = "$2b$12$aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    password_ok = await verify_password(body.password, user.password_hash if user else dummy_hash)

    if not user or not password_ok:
        raise HTTPException(status_code=401, detail="Invalid credentials")

    await crud.set_user_online(db, str(user.user_id), online=True)
    token = create_access_token(str(user.user_id))
    _set_auth_cookie(response, token)
    return {"username": user.username}


@router.get("/me")
async def me(
    user_id: str = Depends(get_current_user),
    db: AsyncSession = Depends(get_db),
) -> dict:
    """Lightweight auth probe — frontend hits this on mount to decide whether
    to render <AuthView> or the authenticated dashboard. Returns 401 via the
    `get_current_user` dependency if the cookie is missing or expired.
    Also returns the username so the UI can display it (avoids exposing the
    user UUID as the display name)."""
    user = await crud.get_user_by_id(db, user_id)
    return {
        "user_id": user_id,
        "username": user.username if user else None,
    }


@router.post("/logout")
async def logout(response: Response, user_id: str = Depends(get_current_user)):
    try:
        await session_manager.terminate(user_id)
    except Exception as e:
        print(f"[Auth] logout terminate error for {user_id}: {e}")
    response.delete_cookie(key=_COOKIE, path="/")
    return {"message": "Logged out"}
