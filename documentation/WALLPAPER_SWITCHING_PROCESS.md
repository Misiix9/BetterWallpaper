# Wallpaper Switching Process - Technical Documentation

> This document describes the exact sequence of events that occurs when a user switches from one wallpaper to another in BetterWallpaper.

---

## Overview

The wallpaper switching process is designed to be:
1. **Instant** - No visible loading delay for the user
2. **Seamless** - Old wallpaper never disappears until new one is ready
3. **Persistent** - Wallpaper survives app close and system reboot

### Key Principle: New Wallpaper Loads IN FRONT

When switching wallpapers:
- **Wallpaper B (new)** spawns IN FRONT of Wallpaper A
- B starts with **opacity 0** (fully transparent/invisible)
- B **fades IN** over A during the transition
- A remains visible behind B until the transition completes
- Once B is fully visible, A is destroyed

---

## Detailed Process Flow

### Stage 1: User Selects a Wallpaper

**Trigger:** User clicks on a wallpaper card in the library grid.

```
┌─────────────────────────────────────────────────────────────┐
│ CURRENT STATE                                               │
│ • Wallpaper A is displayed on desktop                       │
│ • User is browsing the library                              │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ USER ACTION: Clicks on Wallpaper B card                     │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ SYSTEM RESPONSE (Immediate):                                │
│ 1. WallpaperCard emits "selected" signal                    │
│ 2. PreviewPanel updates to show Wallpaper B info            │
│ 3. Preview image of Wallpaper B loads                       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ BACKGROUND PRELOADING (Async):                              │
│ • For linux-wallpaperengine types:                          │
│   - Validate scene.pkg or video file exists                 │
│   - Pre-fetch any remote assets if needed                   │
│ • For static images:                                        │
│   - Load full image into memory cache                       │
│ • For videos:                                               │
│   - Initialize MPV context (paused state)                   │
│   - Decode first frame for instant display                  │
│                                                             │
│ FLAG: preload_ready = false → true when complete            │
└─────────────────────────────────────────────────────────────┘
```

### Stage 2: User Clicks "Set as Wallpaper"

**Trigger:** User clicks the "Set as Wallpaper" button in PreviewPanel.

```
┌─────────────────────────────────────────────────────────────┐
│ USER ACTION: Clicks "Set as Wallpaper" button               │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ PRELOAD CHECK:                                              │
│ • If preload_ready == true:                                 │
│   → Proceed immediately to Stage 3                          │
│ • If preload_ready == false:                                │
│   → Show subtle loading indicator (spinner on button)       │
│   → Wait for preload to complete (typically <1 second)      │
│   → Then proceed to Stage 3                                 │
└─────────────────────────────────────────────────────────────┘
```

### Stage 3: New Wallpaper Process Starts

**Note:** The old wallpaper (A) is STILL running and visible. Wallpaper B starts IN FRONT of A (initially transparent/invisible).

```
┌─────────────────────────────────────────────────────────────┐
│ WALLPAPER B INITIALIZATION:                                 │
│                                                             │
│ For linux-wallpaperengine wallpapers:                       │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 1. Spawn linux-wallpaperengine process:                 │ │
│ │    $ linux-wallpaperengine --screen-root <monitor>      │ │
│ │      --bg <workshop_id> [options...]                    │ │
│ │                                                         │ │
│ │ 2. Process starts rendering IN FRONT of A               │ │
│ │    (initially with opacity 0 or hidden)                 │ │
│ │                                                         │ │
│ │ 3. Process is DETACHED from BetterWallpaper:            │ │
│ │    - Uses setsid() to create new session                │ │
│ │    - Will survive if BetterWallpaper closes             │ │
│ └─────────────────────────────────────────────────────────┘ │
│                                                             │
│ For internal renderers (static/video):                      │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 1. Create new WallpaperWindow (layer-shell)             │ │
│ │ 2. Set z-index ABOVE current wallpaper (in front)       │ │
│ │ 3. Start with opacity 0 (invisible)                     │ │
│ │ 4. Load renderer with preloaded content                 │ │
│ │ 5. Render first frame (still invisible)                 │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ READY CHECK:                                                │
│ • Wallpaper B signals "ready" when:                         │
│   - First frame has been rendered successfully              │
│   - For video: MPV reports playback started                 │
│   - For scene: linux-wallpaperengine is running             │
│                                                             │
│ FLAG: wallpaper_b_ready = true                              │
└─────────────────────────────────────────────────────────────┘
```

### Stage 4: Transition Begins

**Note:** Only starts AFTER Wallpaper B confirms it's ready. B fades IN over A.

```
┌─────────────────────────────────────────────────────────────┐
│ TRANSITION EXECUTION:                                       │
│                                                             │
│ 1. Read user's transition preferences from config:          │
│    - Effect: transitions.default_effect (e.g., "Fade")      │
│    - Duration: transitions.duration_ms (e.g., 500)          │
│    - Easing: transitions.easing (e.g., "easeInOut")         │
│                                                             │
│ 2. For linux-wallpaperengine transitions:                   │
│    ┌───────────────────────────────────────────────────────┐│
│    │ • Wallpaper B is IN FRONT of A (higher layer)         ││
│    │ • B starts with opacity 0 (A fully visible behind)    ││
│    │ • Animate B's opacity: 0 → 1 (B fades IN)             ││
│    │ • A stays at opacity 1 (visible behind B)             ││
│    │ • When B reaches opacity 1, A is fully covered        ││
│    └───────────────────────────────────────────────────────┘│
│                                                             │
│ 3. For internal renderers:                                  │
│    ┌───────────────────────────────────────────────────────┐│
│    │ • Use TransitionEngine with selected effect           ││
│    │ • B window is in front, animates opacity/position     ││
│    │ • Render transition frames at 60fps                   ││
│    │ • Apply easing function to progress                   ││
│    └───────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ TRANSITION PROGRESS (B fades in OVER A):                    │
│                                                             │
│ Time 0ms:    [A visible, B invisible]  B opacity: 0%        │
│ Time 125ms:  [A behind, B 25% visible] B opacity: 25%       │
│ Time 250ms:  [A behind, B 50% visible] B opacity: 50%       │
│ Time 375ms:  [A behind, B 75% visible] B opacity: 75%       │
│ Time 500ms:  [A hidden, B fully visible] B opacity: 100%    │
│                                                             │
│ Visual representation:                                      │
│ ┌────────────────────────────────────────────────────────┐  │
│ │  Start      25%        50%        75%       End        │  │
│ │  ┌───┐     ┌───┐      ┌───┐      ┌───┐     ┌───┐      │  │
│ │  │ A │  →  │A+B│  →   │A+B│  →   │A+B│  →  │ B │      │  │
│ │  └───┘     └───┘      └───┘      └───┘     └───┘      │  │
│ │  B=0%      B=25%      B=50%      B=75%     B=100%     │  │
│ └────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Stage 5: Cleanup

**Trigger:** Transition animation completes.

```
┌─────────────────────────────────────────────────────────────┐
│ CLEANUP OPERATIONS:                                         │
│                                                             │
│ 1. Kill Wallpaper A process:                                │
│    - For linux-wallpaperengine: pkill the old PID           │
│    - For internal: destroy WallpaperWindow A                │
│                                                             │
│ 2. Remove transition overlay (if used)                      │
│                                                             │
│ 3. Update state:                                            │
│    - current_wallpaper = Wallpaper B                        │
│    - previous_wallpaper = Wallpaper A (for undo)            │
└─────────────────────────────────────────────────────────────┘
```

### Stage 6: Persistence

**Trigger:** Immediately after successful wallpaper change.

```
┌─────────────────────────────────────────────────────────────┐
│ SAVE TO CONFIG (config.json):                               │
│                                                             │
│ {                                                           │
│   "state": {                                                │
│     "last_wallpaper": {                                     │
│       "path": "/path/to/wallpaper_b",                       │
│       "monitor": "eDP-1",                                   │
│       "type": "WEScene",                                    │
│       "workshop_id": "3160110208",                          │
│       "options": {                                          │
│         "volume": 50,                                       │
│         "fps": 30,                                          │
│         "silent": false                                     │
│       },                                                    │
│       "set_at": "2026-01-29T19:45:00Z"                      │
│     }                                                       │
│   }                                                         │
│ }                                                           │
│                                                             │
│ This allows:                                                │
│ • Restoring wallpaper on next app launch                    │
│ • Restoring wallpaper after system reboot                   │
│ • Showing "current wallpaper" in UI                         │
└─────────────────────────────────────────────────────────────┘
```

---

## Error Handling

### If New Wallpaper Fails to Load

```
┌─────────────────────────────────────────────────────────────┐
│ ERROR SCENARIO: Wallpaper B fails to initialize             │
│                                                             │
│ Possible causes:                                            │
│ • File not found                                            │
│ • Corrupted file                                            │
│ • linux-wallpaperengine crash                               │
│ • Unsupported format                                        │
│                                                             │
│ RECOVERY:                                                   │
│ 1. Wallpaper A continues running (no interruption)          │
│ 2. Error notification shown to user                         │
│ 3. PreviewPanel shows error state                           │
│ 4. User can try again or select different wallpaper         │
│                                                             │
│ GUARANTEE: User NEVER sees blank screen                     │
└─────────────────────────────────────────────────────────────┘
```

---

## Persistence After Reboot

### Automatic Restoration Flow

```
┌─────────────────────────────────────────────────────────────┐
│ SYSTEM BOOT / USER LOGIN                                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ betterwallpaper-restore.service STARTS                      │
│ (systemd user service, runs after graphical-session.target) │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ RESTORE COMMAND: bwp restore-wallpaper                      │
│                                                             │
│ 1. Read config.json → state.last_wallpaper                  │
│ 2. Verify wallpaper file still exists                       │
│ 3. Spawn linux-wallpaperengine with saved options           │
│ 4. Exit (wallpaper now running independently)               │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ RESULT: Same wallpaper as before reboot                     │
│ • No BetterWallpaper GUI needed                             │
│ • Wallpaper runs as standalone process                      │
└─────────────────────────────────────────────────────────────┘
```

---

## Technical Implementation Notes

### Process Detachment (Daemonization)

When spawning linux-wallpaperengine, we use:

```cpp
// Fork and detach the wallpaper process
std::string command = "setsid linux-wallpaperengine ...";
// OR
std::string command = "nohup linux-wallpaperengine ... &";
// OR (more robust)
// Use g_spawn_async with G_SPAWN_DO_NOT_REAP_CHILD
```

This ensures:
- Process is not a child of BetterWallpaper
- Closing BetterWallpaper doesn't send SIGHUP
- Process survives terminal/session close

### Transition Layer Management

For linux-wallpaperengine wallpapers (external processes):

```
Layer Stack (bottom to top):
┌────────────────────────────────────┐
│ Desktop Background (compositor)    │ ← Layer 0
├────────────────────────────────────┤
│ Wallpaper A (current, visible)     │ ← Layer 1 (gtk4-layer-shell: bottom)
├────────────────────────────────────┤
│ Wallpaper B (new, in front)        │ ← Layer 2 (gtk4-layer-shell: bottom, higher z)
├────────────────────────────────────┤
│ Desktop Windows                    │ ← Normal windows
└────────────────────────────────────┘

During transition:
1. B starts with opacity 0 (fully transparent)
2. B is IN FRONT of A (higher z-index layer)
3. Animate B's opacity: 0.0 → 1.0 (B fades IN)
4. A remains at opacity 1.0 (visible behind B)
5. When transition completes, kill A process
6. B is now the only wallpaper running
```

---

## Summary

| Stage | Description | User Sees |
|-------|-------------|-----------|
| 1 | User selects wallpaper | Preview panel updates |
| 2 | User clicks "Set" | Button confirms (instant) |
| 3 | New wallpaper starts | Nothing changes yet |
| 4 | Transition plays | Smooth fade/slide/etc. |
| 5 | Old wallpaper killed | Only new wallpaper |
| 6 | State saved | Settings preserved |
| Reboot | Restore service runs | Same wallpaper returns |
