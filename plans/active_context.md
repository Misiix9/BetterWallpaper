# ACTIVE CONTEXT (The Scratchpad)

## Current Task
**Executing Task: WebGL/HTML5 Wallpaper Support [Roadmap #1]**

## Context
Phase 1-3 are complete. We are now in Phase 4 (Expansion).
The goal is to enable rendering of HTML5/WebGL wallpapers.

## Plan
1.  Investigate `WallpaperType` detection in `WallpaperLibrary`.
2.  Determine if `WallpaperEngineRenderer` supports HTML/Web types (via `linux-wallpaperengine`).
3.  If yes, enable detection. If no, research `WebKitGTK` integration or external player.
4.  Verify with `hand_of_god.py`.
