import uuid
from datetime import datetime, timezone
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, update
from backend.models.user import User
from backend.services.auth_service import hash_password


async def create_user(db: AsyncSession, username: str, plain_password: str) -> User:
    pwd_hash = await hash_password(plain_password)
    user = User(
        user_id=uuid.uuid4(),
        username=username,
        password_hash=pwd_hash,
    )
    db.add(user)
    await db.commit()
    await db.refresh(user)
    return user


async def get_user_by_username(db: AsyncSession, username: str) -> User | None:
    result = await db.execute(select(User).where(User.username == username))
    return result.scalar_one_or_none()


async def get_user_by_id(db: AsyncSession, user_id: str) -> User | None:
    result = await db.execute(
        select(User).where(User.user_id == uuid.UUID(user_id))
    )
    return result.scalar_one_or_none()


async def set_user_online(db: AsyncSession, user_id: str, online: bool) -> None:
    stmt = (
        update(User)
        .where(User.user_id == uuid.UUID(user_id))
        .values(
            is_online=online,
            last_login=datetime.now(timezone.utc) if online else None,
        )
    )
    await db.execute(stmt)
    await db.commit()
