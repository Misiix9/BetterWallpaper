# BetterWallpaper - Ideas & Improvements

> This document contains ideas for improving existing features. No new major features - only enhancements to current functionality.

---

## UI/UX Improvements

### Wallpaper Cards
- [ ] **Lazy thumbnail loading** - Only generate thumbnails for visible cards
- [ ] **Skeleton loading state** - Show animated skeleton while thumbnail loads
- [ ] **Progressive image loading** - Show low-res thumbnail first, then high-res
- [ ] **Hover preview animation** - Subtle zoom/scale on hover (not jarring)
- [ ] **Badge improvements** - Better positioned favorite star, type indicator
- [ ] **Right-click context menu** - Quick actions (favorite, set, delete, info)

### Preview Panel
- [ ] **Instant metadata display** - Show resolution, file size, type immediately
- [ ] **Larger preview image** - Option to expand preview to full window
- [ ] **Preview zoom/pan** - Allow zooming into wallpaper details
- [ ] **Mini video player controls** - For video wallpapers (play/pause/seek)
- [ ] **Quick actions bar** - Favorite, tag, set - all in one row
- [ ] **Wallpaper history** - Show "previously set" wallpapers

### Settings
- [ ] **Visual feedback** - Toast notifications when settings change
- [ ] **Grouped settings** - Collapsible sections for better organization
- [ ] **Reset to defaults** - Button to reset individual or all settings
- [ ] **Import/Export settings** - Backup and restore configuration
- [ ] **Settings search** - Quick find setting by name

### Library View
- [ ] **Grid density options** - Compact, comfortable, spacious views
- [ ] **Sort options** - By name, date added, date modified, favorites first
- [ ] **Filter persistence** - Remember last filter/search on restart
- [ ] **Bulk operations** - Select multiple wallpapers for batch actions
- [ ] **Drag and drop reorder** - Custom sorting within favorites

### Monitor Layout
- [ ] **Visual monitor preview** - Show current wallpaper on each monitor
- [ ] **Drag to assign** - Drag wallpaper onto monitor preview
- [ ] **Monitor naming** - Custom names for monitors (e.g., "Main", "Secondary")
- [ ] **Quick swap** - Swap wallpapers between monitors

---

## Performance Improvements

### Startup
- [ ] **Lazy view loading** - Don't create views until needed
- [ ] **Cached UI state** - Remember and restore scroll positions
- [ ] **Optimized CSS** - Reduce CSS complexity for faster rendering
- [ ] **Precompiled shaders** - Cache GTK shader compilation

### Library Management
- [ ] **Incremental scanning** - Only scan changed files
- [ ] **File watcher** - Auto-detect new wallpapers in folders
- [ ] **Indexed search** - Build search index for instant results
- [ ] **Thumbnail cache cleanup** - Remove orphaned thumbnails

### Memory
- [ ] **Thumbnail memory limits** - Unload off-screen thumbnails
- [ ] **Image downsampling** - Use smaller images for grid view
- [ ] **Resource pooling** - Reuse GtkWidget instances in grid

### Transitions
- [ ] **GPU-accelerated transitions** - Use OpenGL for effects
- [ ] **Preloaded transitions** - Cache transition resources
- [ ] **Adaptive quality** - Lower quality transitions on slow systems

---

## Wallpaper Management

### Organization
- [ ] **Smart folders** - Auto-organize by type (static, video, scene)
- [ ] **Tags management UI** - Create, edit, delete tags
- [ ] **Tag suggestions** - AI-based tag recommendations
- [ ] **Duplicate detection** - Find and manage duplicate wallpapers

### Preview & Playback
- [ ] **Animated GIF support** - Play GIFs in preview
- [ ] **HDR preview** - Proper HDR rendering for HDR wallpapers
- [ ] **Audio preview** - Preview wallpaper audio before setting

### Import/Export
- [ ] **Drag and drop import** - Drop images directly into library
- [ ] **Batch import progress** - Show progress for large imports
- [ ] **Export wallpaper packs** - Share curated collections
- [ ] **Cloud sync** - Sync library across devices (optional)

---

## System Integration

### Tray Icon
- [ ] **Quick wallpaper switch** - Cycle through favorites from tray
- [ ] **Pause/Resume indicator** - Visual state in tray icon
- [ ] **Tray context menu polish** - Better organization, icons
- [ ] **System theme integration** - Match tray icon to system theme

### Keyboard Shortcuts
- [ ] **Global shortcuts** - System-wide hotkeys (not just in-app)
- [ ] **Shortcut conflicts** - Detect and warn about conflicts
- [ ] **Shortcut profiles** - Different shortcuts for different scenarios

### Notifications
- [ ] **Wallpaper changed notification** - Optional toast when wallpaper changes
- [ ] **Schedule reminders** - Notify when scheduled change occurs
- [ ] **Error notifications** - Clear error messages with actions

---

## Stability & Reliability

### Error Handling
- [ ] **Graceful degradation** - App works even if some features fail
- [ ] **Detailed error logs** - Better logging for troubleshooting
- [ ] **Recovery mode** - Reset to safe defaults if config corrupted
- [ ] **Health check** - Verify all components working on startup

### Data Safety
- [ ] **Config backup** - Auto-backup before changes
- [ ] **Atomic saves** - Write to temp file, then rename
- [ ] **Library verification** - Detect and report missing wallpapers
- [ ] **Safe shutdown** - Ensure all writes complete before exit

---

## Future Considerations (Lower Priority)

### Advanced Features
- [ ] **Wallpaper color extraction** - Extract dominant colors for UI theming
- [ ] **Playlist mode** - Sequential playback of wallpaper sets
- [ ] **Time-based variants** - Day/night versions of same wallpaper
- [ ] **Weather-based selection** - Match wallpaper to current weather

### Community
- [ ] **Share ratings/favorites** - Optional community features
- [ ] **Wallpaper recommendations** - "You might also like" suggestions
- [ ] **Usage statistics** - Track most-used wallpapers

---

## Implementation Notes

### Priority Order (for Phase 4.5)
1. **Transition settings in UI** - Essential for user control
2. **Instant wallpaper setting** - Core UX requirement
3. **Persistence after reboot** - Critical for usability
4. **Performance optimizations** - Quality of life
5. **UI polish** - Nice to have

### Technical Debt to Address
- [ ] Remove commented-out code
- [ ] Consolidate duplicate logic
- [ ] Add unit tests for core functions
- [ ] Document public APIs
- [ ] Standardize error handling patterns

---

*Last updated: 2026-01-29*
