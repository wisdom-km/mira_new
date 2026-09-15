#!/usr/bin/env python3
"""Emit a schemaVersion-2 official-manifest asset object for a Skill folder."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def sha256_file(path: Path) -> tuple[str, int]:
    data = path.read_bytes()
    return hashlib.sha256(data).hexdigest(), len(data)


def posix_rel(root: Path, path: Path) -> str:
    return path.relative_to(root).as_posix()


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate a DirectorDesk skill manifest entry")
    parser.add_argument("--id", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--dir", required=True, type=Path)
    parser.add_argument("--category", default="skill")
    parser.add_argument("--license", default="MIT")
    parser.add_argument("--name-zh", default="")
    parser.add_argument("--name-en", default="")
    parser.add_argument("--author", default="DirectorDesk Community")
    parser.add_argument("--url-prefix", default="")
    args = parser.parse_args()

    root = args.dir.resolve()
    skill = root / "SKILL.md"
    if not skill.is_file():
        raise SystemExit(f"missing {skill}")

    files = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        digest, size = sha256_file(path)
        rel = posix_rel(root, path)
        url = f"{args.url_prefix}{args.id}/{args.version}/{rel}" if args.url_prefix else f"skills/{args.id}/{args.version}/{rel}"
        files.append({"path": rel, "url": url, "sha256": digest, "size": size})

    name = {}
    if args.name_zh:
        name["zh-CN"] = args.name_zh
    if args.name_en:
        name["en"] = args.name_en
    if not name:
        name["en"] = args.id

    asset = {
        "id": args.id,
        "version": args.version,
        "name": name,
        "category": args.category,
        "format": "skill",
        "kind": "skill",
        "entrypoint": "SKILL.md",
        "files": files,
        "license": {"spdx": args.license, "name": args.license},
        "author": {"name": args.author},
    }
    print(json.dumps(asset, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
