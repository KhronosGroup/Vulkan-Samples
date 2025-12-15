#!/usr/bin/env bash
set -euo pipefail

# Fetch the Bistro example assets into the desired assets directory.
# Default target: assets/bistro at the repository root.
# Usage:
#   ./fetch_bistro_assets.sh [target-dir]
# Example:
#   ./fetch_bistro_assets.sh            # clones to assets/bistro

REPO_SSH="git@github.com:gpx1000/bistro.git"
REPO_HTTPS="https://github.com/gpx1000/bistro.git"
TARGET_DIR="${1:-Assets/bistro}"

mkdir -p "$(dirname "${TARGET_DIR}")"

# If directory exists and is a git repo, update it; otherwise clone it.
if [ -d "${TARGET_DIR}/.git" ]; then
  echo "Updating existing bistro assets in ${TARGET_DIR}"
  git -C "${TARGET_DIR}" pull --ff-only
else
  echo "Cloning bistro assets into ${TARGET_DIR}"
  # Try SSH first; if it fails (e.g., no SSH key), fall back to HTTPS.
  if git clone --depth 1 "${REPO_SSH}" "${TARGET_DIR}" 2>/dev/null; then
    :
  else
    echo "SSH clone failed, trying HTTPS"
    git clone --depth 1 "${REPO_HTTPS}" "${TARGET_DIR}"
  fi
fi

# If git-lfs is available, ensure LFS content is pulled
if command -v git >/dev/null 2>&1 && git -C "${TARGET_DIR}" lfs version >/dev/null 2>&1; then
  git -C "${TARGET_DIR}" lfs install --local >/dev/null 2>&1 || true
  git -C "${TARGET_DIR}" lfs pull || true
fi

echo "Bistro assets ready at: ${TARGET_DIR}"
