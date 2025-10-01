#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
复制 Editor 资源目录到目标可执行目录下的 CoronaEditor/ 子目录，并使用本地 Node 环境执行前端构建。
- 通过多次传入 --src-dir 指定多个源目录（如 Backend、Frontend 等）
- 目标根目录由 --dest-root 指定（通常是 $<TARGET_FILE_DIR:...>/CoronaEditor ）
- 然后用 --node-dir 和 --frontend-dir 触发 npm install / npm run build
注意：日志输出为英文，便于在构建日志中检索。
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path
import subprocess
from typing import Optional


def echo(msg: str) -> None:
    print(msg)


def copy_tree(src: Path, dst: Path) -> None:
    if not src.exists():
        echo(f"[editor-copy] Skip (not exists): {src}")
        return
    # Ensure destination directory exists
    dst.mkdir(parents=True, exist_ok=True)
    # Copy content of src into dst/src.name
    target = dst / src.name
    # If target exists, use dirs_exist_ok=True to merge
    echo(f"[editor-copy] Copying: {src} -> {target}")
    # Avoid copying heavy transient folders
    def _ignore(dirpath: str, names: list[str]) -> set[str]:
        ignored = {"node_modules", "dist", ".vite", ".turbo", ".cache"}
        return {n for n in names if n in ignored}
    try:
        shutil.copytree(src, target, dirs_exist_ok=True, ignore=_ignore)
    except Exception as e:
        print(f"[editor-copy] ERROR copying {src} -> {target}: {e}")


def run(cmd: list[str], cwd: Path | None = None, env: dict | None = None) -> int:
    print(f"$ {' '.join(str(c) for c in cmd)}")
    if cwd:
        print(f"  (cwd: {cwd})")
    try:
        p = subprocess.run([str(c) for c in cmd], cwd=str(cwd) if cwd else None, env=env, check=False)
        return p.returncode
    except FileNotFoundError as e:
        print(f"ERROR: Command not found: {cmd[0]} -> {e}")
        return 127


def stream_run(cmd: list[str], cwd: Optional[Path] = None, env: Optional[dict] = None) -> int:
    # Simplified: just run and let the console handle buffering/encoding
    print(f"$ {' '.join(str(c) for c in cmd)}")
    if cwd:
        print(f"  (cwd: {cwd})")
    try:
        p = subprocess.run([str(c) for c in cmd], cwd=str(cwd) if cwd else None, env=env, check=False)
        return p.returncode
    except FileNotFoundError as e:
        print(f"ERROR: Command not found: {cmd[0]} -> {e}")
        return 127


def maybe_run_npm(frontend_dir: Path, node_dir: Path) -> int:
    if not frontend_dir.exists():
        echo(f"[npm-build] Frontend directory not found: {frontend_dir}; skip build")
        return 0
    npm_cmd = node_dir / ("npm.cmd" if os.name == "nt" else "npm")
    if not npm_cmd.exists():
        echo(f"[npm-build] npm not found at: {npm_cmd}; skip build")
        return 0
    env = os.environ.copy()
    env["PATH"] = str(node_dir) + os.pathsep + env.get("PATH", "")
    # Keep environment unchanged
    echo("[npm-build] Installing dependencies...")
    rc = stream_run([str(npm_cmd), "install"], cwd=frontend_dir, env=env)
    if rc != 0:
        print(f"CMake Warning: [npm-build] 'npm install' failed (exit code {rc}); continuing without frontend build.", flush=True)
        return 0
    echo("[npm-build] Building frontend...")
    rc = stream_run([str(npm_cmd), "run", "build"], cwd=frontend_dir, env=env)
    if rc != 0:
        print(f"CMake Warning: [npm-build] 'npm run build' failed (exit code {rc}); continuing without frontend build.", flush=True)
        return 0
    echo("[npm-build] Frontend build completed successfully.")
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Copy editor resources and run npm build")
    ap.add_argument("--dest-root", required=True, help="Destination root directory (CoronaEditor under target dir)")
    ap.add_argument("--src-dir", action="append", default=[], help="Source directory to copy (can be specified multiple times)")
    ap.add_argument("--frontend-dir", required=True, help="Frontend directory path for npm build")
    ap.add_argument("--node-dir", required=True, help="Bundled Node directory path")
    args = ap.parse_args(argv)

    dest_root = Path(args.dest_root).resolve()
    src_dirs = [Path(p).resolve() for p in args.src_dir]
    frontend_dir = Path(args.frontend_dir).resolve()
    node_dir = Path(args.node_dir).resolve()

    echo(f"[editor-copy] Dest root: {dest_root}")
    dest_root.mkdir(parents=True, exist_ok=True)

    for s in src_dirs:
        copy_tree(s, dest_root)

    # Run npm build last
    return maybe_run_npm(frontend_dir, node_dir)


if __name__ == "__main__":
    sys.exit(main())
