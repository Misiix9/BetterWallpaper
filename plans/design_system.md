# DESIGN SYSTEM: Monochrome Glass

## 1. The "Monochrome Glass" Palette

**Core Philosophy:** A strict, high-contrast monochrome environment. Color is removed to focus entirely on the artwork (wallpapers). Depth is created through layers, blur, and lighting, not hue.

### Surfaces (The Layout)
*   **Deep Void (App Background):** `#0A0A0A` (Deepest layer, not #000000)
*   **Carbon (Secondary Background):** `#121212` (Sidebar/Panels)
*   **Glass Surface (Cards/Overlays):** `rgba(30, 30, 30, 0.6)` + `backdrop-filter: blur(24px)`
    *   *Note: Using dark glass to maintain contrast.*
*   **Border (Subtle):** `#262626` (Separator lines)
*   **Border (Highlight):** `rgba(255, 255, 255, 0.1)`

### Typography & Content
*   **Text Primary (Headings):** `#EDEDED` (High readability, not #FFFFFF)
*   **Text Secondary (Body):** `#A1A1AA` (Muted gray)
*   **Text Tertiary (Meta/Disabled):** `#52525B`
*   **Interactive/Accent:** `#E5E5E5` (Actionable elements)
*   **Selection Glow:** `0 0 15px rgba(255, 255, 255, 0.08)`

### Semantic Status (Desaturated)
*   **Error:** `#7F1D1D` (Deep Red) -> Text: `#FECACA`
*   **Success:** `#064E3B` (Deep Green) -> Text: `#A7F3D0`
*   **Steam/Download:** `#172026` (Steam Dark) -> Border: `#313F4A`

## 2. Typography & Iconography

### Font Stack
**Primary:** `Geist Sans` (or `Inter` with tight tracking).
*   **H1 (Headers):** 20px / Bold / Tracking -0.02em
*   **H2 (Subheaders):** 16px / Medium
*   **Body:** 13px / Regular / Height 1.5
*   **Mono (Code/Paths):** `Geist Mono` / `JetBrains Mono`

### Iconography
**Style:** Thin, Geometric, Outlined (1.5px stroke width).
*   **Library:** Phosphor Icons / Heroicons (Outline set).
*   **Size:** 16px (Small), 20px (Standard), 24px (Large).

## 3. Component Architecture

### The Wallpaper Card
A precise window into the content.
*   **Container:** `aspect-ratio: 16/9`.
*   **Background:** `#171717`.
*   **Corner Radius:** `12px` (Smooth curvature).
*   **Border:** `1px solid #262626`.
*   **Image:** Object-fit cover.
*   **Hover State:**
    *   Image scales `1.05x`.
    *   Border changes to `#52525B`.
    *   **Title Overlay:** Gradient fade from bottom (`transparent` to `#0A0A0A`) appears. Text slides up 4px.
*   **Selection State:** Border becomes `#E5E5E5` (2px). Soft outer glow.

### The Sidebar (Navigation)
*   **Style:** semi-transparent Glass Dock (Left side).
*   **Width:** 240px.
*   **Background:** `rgba(10, 10, 10, 0.8)` with Blur 30px.
*   **Separator:** Vertical border `1px solid #262626` on the right.
*   **Items:**
    *   Active: `#262626` background pill, Text `#EDEDED`, Icon `#EDEDED`.
    *   Inactive: Transparent, Text `#A1A1AA`.

### The "First Run" Wizard
A focused, modal experience. Center screen.
*   **Stage 1 (Welcome):** Large Logo, "Get Started" button (Solid `#E5E5E5` background, `#0A0A0A` text).
*   **Stage 2 (Monitors):** Visual diagram. User clicks their monitor layout. Selected monitor glows.
*   **Stage 3 (Import):** "Scan Local Folder" vs "Open Workshop".
*   **Stage 4 (Done):** "Launch BetterWallpaper".

### The Mini-Player (Tray Menu)
A floating command center.
*   **Dimensions:** 300px width.
*   **Background:** Heavy Blur Glass (`backdrop-filter: blur(40px)`). Background `#0A0A0A` at 90% opacity.
*   **Layout:**
    *   Top: Current Wallpaper Thumb + Title.
    *   Middle: Play/Pause | Prev | Next (Large hit targets).
    *   Bottom: Volume Slider + Settings Icon.

## 4. Interaction Design

### Physics ("The Snappy Feel")
*   **Standard Transition:** `cubic-bezier(0.2, 0, 0, 1)` (Power ease).
*   **Duration:** `200ms`.
*   **Response:** Animations trigger *immediately* on input (mousedown), no delays.

### Feedback Systems
*   **Steam Download:**
    *   Instead of a generic bar, a thin line at the bottom of the card fills from left to right (`#EDEDED`).
    *   Icon spins gently.
*   **Auto-Tagging (AI):**
    *   Tags appear with a "shimmer" effect (`skeleton-loader`) while processing.
    *   Once loaded, they fade in (`opacity: 0` -> `1`).

### Keybinds UI
*   **Layout:** Clean Data Grid.
*   **Appearance:** Alternating row colors (Striped: Transparent / `#121212`).
*   **Editing:** Click a cell -> It becomes focused input -> Press key -> Flash Green (Success) -> Save.

## 5. Technical Implementation Notes
*   **CSS Variables:**
    *   `--bg-void: #0A0A0A`
    *   `--glass-surface: rgba(30, 30, 30, 0.6)`
    *   `--text-primary: #EDEDED`
*   **Tailwind Config:**
    *   Extend colors with 'void', 'carbon', 'glass'.
    *   Add `backdrop-blur` utilities.
*   **Layering:**
    *   Z-Index 10: Background/Grid.
    *   Z-Index 20: Sidebar (Glass).
    *   Z-Index 50: Overlays/Modals.
