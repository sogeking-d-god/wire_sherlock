import os
import shutil
from pathlib import Path

WORKSPACE_ROOT = Path("/tmp/wiresherlock/sessions")


def root() -> Path:
    WORKSPACE_ROOT.mkdir(parents=True, exist_ok=True)
    return WORKSPACE_ROOT


def create(session_uuid: str) -> Path:
    path = root() / session_uuid
    os.makedirs(path, mode=0o700, exist_ok=False)
    (path / "state").mkdir(mode=0o700, exist_ok=False)
    return path


def destroy(session_uuid: str) -> None:
    path = root() / session_uuid
    shutil.rmtree(path, ignore_errors=True)


def destroy_path(path: Path) -> None:
    shutil.rmtree(path, ignore_errors=True)


def purge_all() -> None:
    shutil.rmtree(WORKSPACE_ROOT, ignore_errors=True)
