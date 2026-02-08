# BetterWallpaper — Copilot Instructions

## Architecture Overview

BetterWallpaper is a C++20 animated wallpaper manager for Linux (Wayland-first, Hyprland/Sway primary targets). It comprises five CMake targets built from `src/`:

| Target | Directory | Links | Role |
|--------|-----------|-------|------|
| `bwp_core` | `src/core/` | nlohmann_json, mpv, curl, wayland | Static library — all headless business logic |
| `bwp_ipc` | `src/core/ipc/` | GIO (Linux) | IPC library — D-Bus service/client on Linux, Named Pipes on Windows |
| `betterwallpaper` | `src/gui/` | bwp_core, GTK4, libadwaita, gtk4-layer-shell | GUI executable |
| `betterwallpaper-daemon` | `src/daemon/` | bwp_core (no GTK) | Headless background service |
| `unit_tests` | `tests/` | bwp_core | Test binary |

**Hard rule:** `src/core/` must never include GTK headers. GUI code in `src/gui/` consumes core via the `bwp::` namespace.

### Key Singletons (Meyer's pattern)
- `ConfigManager` — JSON config at `~/.config/betterwallpaper/config.json`, uses `std::recursive_mutex`, atomic file saves (write-tmp-then-rename)
- `WallpaperManager` — Owns renderers, dispatches to `WallpaperEngineRenderer` (fork-exec of external `linux-wallpaperengine`), `VideoRenderer` (mpv), or `StaticRenderer`
- `MonitorManager` — Binds `wl_output` directly (not GDK) to get real monitor names like `DP-1`

**Mutex lock order:** Config → WallpaperLibrary → Renderer. Never reverse.

### IPC & Platform Abstraction
Interfaces (`IIPCService`, `IIPCClient`, `IWallpaperSetter`) with compile-time `#ifdef _WIN32` factory dispatch. Factories are header-only in `src/core/ipc/`. `WallpaperSetterFactory` uses chain-of-responsibility: tries each platform setter's `isSupported()` in priority order.

### External Process Spawning
`linux-wallpaperengine` and `ffmpeg` are spawned via fork-exec, not linked. Wallpaper processes are detached with `setsid()` so they survive GUI closure. Crash recovery: 3 fast crashes (<10s apart) marks a wallpaper as "Broken".

## Agent Skills (MANDATORY)

Before starting any task, check `/home/onxy/.agents/skills/` for an applicable skill and read its `SKILL.md` before proceeding. Key mappings for this project:

| Trigger | Skill |
|---------|-------|
| Any new feature, component, or behavior change | `brainstorming` |
| Writing a multi-step implementation plan | `writing-plans` |
| Executing an existing plan | `executing-plans` |
| Multiple independent tasks in one session | `subagent-driven-development` |
| 2+ independent tasks that can run without shared state | `dispatching-parallel-agents` |
| Bug, test failure, or unexpected behavior | `systematic-debugging` |
| Implementing any feature or bugfix | `test-driven-development` |
| About to claim work is done / before committing | `verification-before-completion` |
| Code review feedback received | `receiving-code-review` |
| Completed a task, ready for review | `requesting-code-review` |
| Feature work needing isolation | `using-git-worktrees` |
| Finishing a branch (merge/PR/cleanup) | `finishing-a-development-branch` |
| Writing docs, proposals, specs | `doc-coauthoring` |
| Creating generative/algorithmic art (p5.js, etc.) | `algorithmic-art` |
| Applying Anthropic brand colors/typography | `brand-guidelines` |
| Creating visual art, posters, static designs (.png/.pdf) | `canvas-design` |
| Creating or editing Word documents (.docx) | `docx` |
| Building web components, pages, dashboards, or UI | `frontend-design` |
| Writing internal comms (status reports, updates, FAQs) | `internal-comms` |
| Building MCP servers for LLM tool integration | `mcp-builder` |
| Reading, creating, editing, or manipulating PDF files | `pdf` |
| Creating or editing PowerPoint presentations (.pptx) | `pptx` |
| Creating new skills or editing existing ones | `skill-creator` |
| Creating animated GIFs optimized for Slack | `slack-gif-creator` |
| Styling artifacts with a predefined or custom theme | `theme-factory` |
| Starting any conversation / finding applicable skills | `using-superpowers` |
| Building complex multi-component HTML artifacts | `web-artifacts-builder` |
| Testing local web apps with Playwright (screenshots, logs) | `webapp-testing` |
| Creating or verifying skill definitions | `writing-skills` |
| Working with spreadsheet files (.xlsx, .csv, .tsv) | `xlsx` |

**Always invoke the skill file (`read_file` on the `SKILL.md`) before acting.** Do not skip skills even for "simple" tasks — the brainstorming skill, for example, must run before any creative/feature work.

## Build & Run

```bash
# Full build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Install (user-local to ~/.local/bin)
./install.sh

# Run tests
cd build && ctest
```

Dependencies: `gtk4`, `libadwaita-1`, `gtk4-layer-shell-0`, `mpv`, `wayland-client`, `wayland-protocols`, `libcurl`, `gio-2.0`. nlohmann_json is fetched via CMake FetchContent. Install script: `install_deps.sh` (general) or `install_deps_pop.sh` (Pop!_OS).

## Coding Conventions

- **C++20** required, `#pragma once` on all headers
- **Namespace:** `bwp::module` — e.g. `bwp::config`, `bwp::ipc`, `bwp::wallpaper`, `bwp::gui`
- **Naming:** PascalCase classes, camelCase methods, `m_` prefix for members, `I` prefix for interfaces (`IIPCService`)
- **CMake sources:** explicitly listed per-file — no `file(GLOB ...)`
- **Logging:** use `LOG_DEBUG`, `LOG_INFO`, `LOG_ERROR` macros (wrap `bwp::utils::Logger`); `LOG_SCOPE_AUTO()` for function tracing
- **Error codes:** `bwp::utils::ErrorCode` enum with categorized ranges (File 10-19, Wallpaper 20-29, Monitor 30-39, etc.)
- **Smart pointers:** `std::unique_ptr` from factories, custom `GtkPtr<T>` RAII wrapper for GObject ref-counting (`src/core/utils/GtkPtr.hpp`)
- **GTK usage:** C API wrapped in C++ classes (composition, not inheritance) — project does NOT use gtkmm
- **Platform code:** `#ifdef _WIN32` / `#else` blocks in both C++ and CMake

## UI Design Rules ("Liquid Glass")

The UI follows a strict **no-color policy** — the interface is grayscale only. Color comes from wallpaper content or semantic status (red=error, green=success, gold=favorite).

Key CSS variables: `--void: #0A0A0A`, `--carbon: #121212`, `--glass-medium: rgba(20,20,20,0.75)`, `--text-primary: #F0F0F0`, `--text-secondary: #A8A8B3`. All animations use spring physics / `cubic-bezier(0.2, 0, 0, 1)` at 200ms. See `plans/design_system.md` and `data/ui/style.css`.

**Do not add color to the UI.** If a new widget needs styling, use existing glass/carbon surface layers.

## Key Files & Entry Points

- `src/gui/main.cpp` — GUI entry, orchestrates singleton init order
- `src/daemon/main.cpp` — Daemon entry
- `src/core/config/ConfigManager.hpp` — Central config (recursive_mutex, JSON)
- `src/core/wallpaper/` — WallpaperManager, WallpaperLibrary, LibraryScanner, renderers
- `src/core/ipc/` — D-Bus service, IPC interfaces and factories
- `src/core/monitor/MonitorManager.cpp` — Wayland `wl_output` binding
- `src/core/utils/` — Logger, Error codes, Constants, GtkPtr, ScopeTracer
- `documentation/AGENT_HANDOVER.md` — Comprehensive architecture reference
- `plans/design_system.md` — Full UI specification

## Known Gotchas

- Steam Workshop scanner has hardcoded paths in `LibraryScanner.cpp` — use `$HOME` not literal paths
- `WallpaperEngineRenderer` injects `LD_LIBRARY_PATH` for local `libGLEW` — fragile, see `libGLEW.so.2.2` in project root
- Hyprland integration shells out via `system("hyprctl ...")` — should use Unix socket directly
- Tests are placeholder (`assert(true)`) — no test framework yet; `tests/unit/` and `tests/integration/` directories exist but are empty
- Large libraries (5000+ wallpapers) cause ~1.5s startup delay in `WallpaperLibrary::load()`
