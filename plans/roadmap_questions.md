# ROADMAP QUESTIONNAIRE: DEFINING THE FUTURE

## A. The "Next Big Thing" (New Features)
1.  **Interactive Wallpapers**: We have Video support. Should we add support for **Web-based (HTML5/WebGL)** wallpapers? (Allows mouse interaction, games, etc.)
2.  **Smart Scheduling**: Should we add an "Environmental" scheduler? (e.g., Change wallpaper at Sunset, or when it's Raining - requires Weather API).
3.  **Monitor Profiles**: Do you want distinct "Profiles" per monitor? (e.g., Left screen = Gaming wallpapers, Right screen = Productivity).
4.  **Scene Editor**: Should we build a simple **In-App Editor** to let you create basic wallpapers (add text/widgets to images)?
5.  **Audio Visualization**: Should we support wallpapers that react to your music/system audio? (bar visualizers, pulse effects).
6.  **System Stats Widget**: Do you want an overlay feature to show CPU/RAM usage directly on the wallpaper?
7.  **Screensaver Mode**: Should BetterWallpaper also act as your system screensaver when idle?
8.  **AI Generation**: Should we integrate a local AI (Stable Diffusion via API) to generate wallpapers on the fly?

## B. Visuals & "Vibe" (UI/Design)
9.  **Aesthetic Direction**: The current UI is functional. Do you want to move towards a **"Glassmorphism"** (Frosted Glass/Blur) look?
10. **Accent Colors**: Should the app UI automatically extract colors from the *current wallpaper* to theme itself?
11. **Sidebar Behavior**: Do you prefer the sidebar to be **Sticky** (Always visible) or **Collapsible** (Hamburger menu)?
12. **Compact Mode**: Do you want a "Mini Player" view that floats on top of other windows?
13. **Grid Density**: For the Library view, do you prefer **Large Cards** (cleaner) or **Dense Grid** (see more items)?
14. **Animations**: For UI interactions, do you want **Snappy/Instant** feel or **Fluid/Cinematic** motion?
15. **Dark Mode**: You mentioned "follow system". If system is Light mode, should we allow forcing Dark Mode anyway?

## C. User Experience (UX)
16. **First Run Wizard**: Should we add a 3-step tutorial for new users (Import Wallpapers -> Select Monitor -> Done)?
17. **Smart Playlists**: Do you want "Dynamic Playlists"? (e.g., A playlist that automatically adds any wallpaper with tag "Anime").
18. **Tagging UX**: Should we use **AI Image Recognition** to auto-tag imported wallpapers? (e.g., "Forest", "Dark").
19. **Global Hotkeys**: Do you want a built-in Hotkey configuration UI for "Next Wallpaper", "Mute", "Hide", etc.?
20. **Search**: Should the search bar optionally query the **Steam Workshop** directly, or only local files?
21. **Favorites**: Should "Favoriting" a wallpaper automatically move it to a "Favorites" playlist?
22. **Tray Behavior**: You chose "Minimize to Tray". On single-click of the tray icon, should it: **Open App** or **Show Mini-Controls**?
23. **Notification**: Should we send a desktop notification when the wallpaper changes?

## D. Integration & Advanced
24. **Steam Workshop**: We have a client. Should we also support **Uploading/Publishing** your own wallpapers to Workshop?
25. **Cloud Sync**: Do you want to sync your library metadata (ratings/playlists) via a cloud provider? (e.g., Google Drive / Nextcloud).
26. **Discord RPC**: Should we show "Watching: Forest Sunset" on your Discord status?
27. **Home Assistant**: Do you want MQTT support to control wallpapers from your Smart Home ecosystem?
28. **CLI JSON**: For the CLI, should we output raw JSON? (Useful for wrapping scripts/integrations like `waybar`).
29. **Battery Saver**: On low battery, should we automatically switch to a **Black/Static** wallpaper to save power?
30. **Plugin System**: Do you want to allow users to write **Lua/Python scripts** to control the daemon?
