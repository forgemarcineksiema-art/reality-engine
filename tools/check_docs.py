"""Validate local documentation, declared identifiers and immutable archive files.

Uses only Python's standard library. It does not validate engine behavior or
contact external websites. Run from any directory, or pass --root for fixtures.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path, PurePosixPath
from urllib.parse import unquote, urlsplit


REQUIRED_DOCUMENTS = {"README.md", "AGENTS.md", "engineering-rules.md"}
DOCUMENT_ROLES = {
    "entry", "normative", "proposal", "planned", "current-state",
    "decision-register", "open", "research", "template", "archive-index",
    "verification",
}
IGNORED_DIRECTORIES = {".git", ".venv", "build", "node_modules", "__pycache__"}
LINK = re.compile(r"!?\[[^\]\n]*\]\(([^)\n]+)\)")
HEADING = re.compile(r"^#{1,6}\s+(.+?)\s*#*\s*$", re.MULTILINE)
REFERENCE_LINK = re.compile(r"!?\[[^\]\n]+\]\[[^\]\n]*\]|^\s{0,3}\[[^\]\n]+\]:", re.MULTILINE)
HTML_LINK = re.compile(r"<\s*(?:a|img)\b[^>]*\b(?:href|src)\s*=", re.IGNORECASE)


def prose(text: str) -> str:
    """Exclude fenced examples from link and identifier validation."""
    lines = []
    fence = None
    for line in text.splitlines():
        match = re.match(r"^\s*(`{3,}|~{3,})", line)
        if match:
            marker = match.group(1)
            if fence is None:
                fence = marker
            elif marker[0] == fence[0] and len(marker) >= len(fence):
                fence = None
            lines.append("")
        else:
            lines.append(line if fence is None else "")
    return "\n".join(lines)


def anchors(text: str) -> set[str]:
    counts: dict[str, int] = {}
    result = set()
    for heading in HEADING.findall(prose(text)):
        base = re.sub(r"[^\w\- ]", "", heading.casefold())
        base = re.sub(r"\s", "-", base)
        index = counts.get(base, 0)
        result.add(base if index == 0 else f"{base}-{index}")
        counts[base] = index + 1
    return result


def workspace_path(root: Path, relative: str) -> Path:
    if not isinstance(relative, str) or not relative or "\\" in relative:
        raise ValueError(f"invalid repository path: {relative!r}")
    candidate = (root / relative).resolve()
    if Path(relative).is_absolute() or not candidate.is_relative_to(root):
        raise ValueError(f"path escapes repository: {relative}")
    if str(PurePosixPath(relative)) != relative or ".." in PurePosixPath(relative).parts:
        raise ValueError(f"non-canonical repository path: {relative}")
    return candidate


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path.name}")
    return value


def validate(root: Path) -> list[str]:
    root = root.resolve()
    errors: list[str] = []
    try:
        manifest = read_json(root / "docs/documentation-map.json")
        if manifest.get("version") != 1:
            raise ValueError("unsupported documentation map version")
        entries = manifest["documents"]
        id_groups = manifest["identifiers"]
        if not isinstance(entries, list) or not isinstance(id_groups, list):
            raise ValueError("documents and identifiers must be arrays")
        if not id_groups:
            raise ValueError("identifiers must not be empty")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        return [f"documentation map: {exc}"]

    registered: set[str] = set()
    registered_locations: set[Path] = set()
    topics: set[str] = set()
    texts: dict[str, str] = {}
    for entry in entries:
        try:
            relative = entry["path"]
            path = workspace_path(root, relative)
            topic = entry["topic"]
            if not isinstance(topic, str) or not topic:
                raise ValueError("each document needs a role and topic")
            if entry["role"] not in DOCUMENT_ROLES:
                raise ValueError(f"unknown document role: {entry['role']}")
            if relative in registered:
                errors.append(f"duplicate document: {relative}")
            if path in registered_locations:
                errors.append(f"duplicate physical document: {relative}")
            if topic in topics:
                errors.append(f"duplicate topic owner: {topic}")
            registered.add(relative)
            registered_locations.add(path)
            topics.add(topic)
            texts[relative] = path.read_text(encoding="utf-8-sig")
            if "\ufffd" in texts[relative]:
                errors.append(f"replacement character: {relative}")
        except (OSError, ValueError, KeyError, TypeError) as exc:
            errors.append(f"document entry: {exc}")

    for relative in sorted(REQUIRED_DOCUMENTS - registered):
        errors.append(f"required document not registered: {relative}")

    archived: set[str] = set()
    try:
        archive = read_json(workspace_path(root, manifest["archive_manifest"]))
        if archive.get("version") != 1 or not isinstance(archive["files"], list):
            raise ValueError("invalid archive manifest")
        for entry in archive["files"]:
            relative = entry["path"]
            path = workspace_path(root, relative)
            if relative in archived or relative in registered:
                errors.append(f"duplicate archive registration: {relative}")
            archived.add(relative)
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            if digest != entry["sha256"]:
                errors.append(f"archive checksum mismatch: {relative}")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        errors.append(f"archive manifest: {exc}")

    for path in sorted(root.rglob("*.md")):
        relative = path.relative_to(root)
        if any(part in IGNORED_DIRECTORIES for part in relative.parts):
            continue
        name = relative.as_posix()
        if name not in registered and name not in archived:
            errors.append(f"unregistered Markdown document: {name}")

    for relative, text in texts.items():
        body = prose(text)
        if REFERENCE_LINK.search(body):
            errors.append(f"{relative}: unsupported reference link; use inline Markdown links")
        if HTML_LINK.search(body):
            errors.append(f"{relative}: unsupported HTML link; use inline Markdown links")
        for target in LINK.findall(body):
            target = target.strip()
            if target.startswith("<") and target.endswith(">"):
                target = target[1:-1]
            try:
                parsed = urlsplit(target)
                if parsed.scheme in {"https", "http", "mailto"}:
                    continue
                if parsed.scheme or parsed.netloc:
                    raise ValueError(f"unsupported link scheme: {target}")
                destination = (
                    root / relative
                    if not parsed.path
                    else (root / relative).parent / unquote(parsed.path)
                ).resolve()
                if not destination.is_relative_to(root):
                    raise ValueError(f"link escapes repository: {target}")
                if not destination.exists():
                    raise ValueError(f"missing local link: {target}")
                if parsed.fragment and destination.suffix.lower() == ".md":
                    content = destination.read_text(encoding="utf-8-sig")
                    if unquote(parsed.fragment) not in anchors(content):
                        raise ValueError(f"missing anchor: {target}")
            except (OSError, ValueError) as exc:
                errors.append(f"{relative}: {exc}")

    for group in id_groups:
        try:
            prefix = group["prefix"]
            pattern = group["pattern"]
            definition_file = group["definition_file"]
            definition_text = texts[definition_file]
            valid = re.compile(rf"(?:{pattern})\Z")
            tokens = re.compile(rf"\b{re.escape(prefix)}-?[0-9][A-Za-z0-9-]*\b")
            defined: set[str] = set()
            for heading in HEADING.findall(prose(definition_text)):
                token = heading.split(maxsplit=1)[0]
                if valid.fullmatch(token):
                    if token in defined:
                        errors.append(f"duplicate identifier definition: {token}")
                    defined.add(token)
            if not defined:
                errors.append(f"no identifier definitions: {prefix}")
            for relative, text in texts.items():
                if relative != definition_file:
                    for heading in HEADING.findall(prose(text)):
                        token = heading.split(maxsplit=1)[0]
                        if valid.fullmatch(token):
                            errors.append(f"{relative}: identifier defined outside owner: {token}")
                # Link destinations contain arbitrary filenames, not contract references.
                # Preserve labels so an undefined identifier in visible prose still fails.
                identifier_body = LINK.sub(
                    lambda match: match.group(0).split("](", 1)[0] + "]", prose(text)
                )
                for token in set(tokens.findall(identifier_body)):
                    if not valid.fullmatch(token) or token not in defined:
                        errors.append(f"{relative}: undefined identifier: {token}")
        except (ValueError, KeyError, TypeError, re.error) as exc:
            errors.append(f"identifier group: {exc}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root", type=Path, default=Path(__file__).resolve().parents[1]
    )
    args = parser.parse_args()
    errors = validate(args.root)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        print(f"FAIL: {len(errors)} documentation issue(s).")
        return 1
    print("PASS: documentation registry, local links, IDs and archive checksums.")
    print("External websites and engine behavior were not tested.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
