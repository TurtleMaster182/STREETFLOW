#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$project_dir/build" --parallel
cmake --install "$project_dir/build" --prefix "$HOME/.local"
printf '\nInstalled %s/.local/bin/streetflow\n' "$HOME"
case ":$PATH:" in
  *":$HOME/.local/bin:"*) printf 'Run: streetflow\n' ;;
  *) printf 'Add this to your shell configuration, then open a new terminal:\nexport PATH="$HOME/.local/bin:$PATH"\n' ;;
esac
