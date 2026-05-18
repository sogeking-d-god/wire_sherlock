from fastapi import Cookie, HTTPException, status
from jose import JWTError
from backend.services.auth_service import decode_access_token


async def get_current_user(access_token: str | None = Cookie(default=None)) -> str:
    """
    FastAPI dependency for protected routes.

    Reads the HttpOnly cookie set at login, validates the JWT,
    and returns the user_id string.  Raises HTTP 401 on any failure.

    Usage in a router:
        @router.get("/something")
        async def my_route(user_id: str = Depends(get_current_user)):
            ...  # user_id is guaranteed valid and belongs to the caller
    """
    if not access_token:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Not authenticated",
        )
    try:
        return decode_access_token(access_token)
    except JWTError:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid or expired token",
        )
