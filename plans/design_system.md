# DESIGN SYSTEM: Liquid Glass v2.0

**Evolution:** From "Monochrome Glass" (static, flat) to "Liquid Glass" (fluid, alive, tactile).

---

## 1. The "Liquid Glass" Philosophy

**Core Tenets:**
1. **Nothing is Static** — Every element responds to interaction with fluid motion.
2. **Depth Through Light** — We create depth using glows, shadows, and luminous borders, not color.
3. **Spring Physics** — Animations feel organic, bouncy, and satisfying, never robotic.
4. **Tactile Feedback** — Clicking feels like pressing glass. Hover feels like heat.
5. **The Content is King** — The UI recedes; wallpapers provide the only color.

---

## 2. The Palette (No-Color Policy Extended)

The interface remains strictly grayscale. Color serves **only** two purposes:
1. **The Content:** Wallpapers provide the color.
2. **Semantic Status:** Red=Error, Green=Success, Gold=Favorite.

### Surface Layers (Depth Stack)

| Layer | Variable | Value | Usage |
|-------|----------|-------|-------|
| Void | `--void` | `#0A0A0A` | The deepest abyss. App background. |
| Carbon | `--carbon` | `#121212` | Sidebars, panels. Slightly elevated. |
| Carbon Elevated | `--carbon-elevated` | `#1A1A1A` | Hover states, subtle lift. |
| Obsidian | `--obsidian` | `#1E1E1E` | Cards at rest. |

### Glass Materials

| Material | Variable | Value | Usage |
|----------|----------|-------|-------|
| Dark Glass | `--glass-dark` | `rgba(10, 10, 10, 0.85)` | Heavy panels, modals. |
| Medium Glass | `--glass-medium` | `rgba(20, 20, 20, 0.75)` | Floating elements. |
| Light Glass | `--glass-light` | `rgba(30, 30, 30, 0.65)` | Overlays, tooltips. |
| Luminous Glass | `--glass-luminous` | `rgba(40, 40, 40, 0.55)` | Active/focused surfaces. |

### Typography Scale

| Style | Variable/Class | Size | Weight | Tracking | Usage |
|-------|----------------|------|--------|----------|-------|
| Primary | `--text-primary` | — | — | — | `#F0F0F0` Headers, emphasis. |
| Secondary | `--text-secondary` | — | — | — | `#A8A8B3` Body text. |
| Tertiary | `--text-tertiary` | — | — | — | `#6B6B76` Hints, disabled. |
| Ghost | `--text-ghost` | — | — | — | `#3A3A42` Placeholders. |
| Title 1 | `.title-1` | 28px | 700 | 0.02em | Page headers. |
| Title 2 | `.title-2` | 22px | 600 | 0.015em | Section headers. |
| Title 3 | `.title-3` | 18px | 600 | 0.01em | Card titles. |
| Title 4 | `.title-4` | 15px | 600 | 0.005em | Subsections. |
| Heading | `.heading` | 11px | 700 | 0.12em | ALL CAPS labels. |
| Body | `.body` | 14px | 400 | — | Content text. |
| Caption | `.caption` | 12px | 500 | — | Metadata, timestamps. |

### Border System

| Purpose | Variable | Value |
|---------|----------|-------|
| Void | `--border-void` | `rgba(255, 255, 255, 0.03)` |
| Subtle | `--border-subtle` | `rgba(255, 255, 255, 0.06)` |
| Visible | `--border-visible` | `rgba(255, 255, 255, 0.10)` |
| Highlight | `--border-highlight` | `rgba(255, 255, 255, 0.15)` |
| Glow | `--border-glow` | `rgba(255, 255, 255, 0.25)` |
| Selection | `--border-selection` | `#E5E5E5` |

---

## 3. Animation Physics

**The Secret Sauce:** Spring-based animations create life.

### Timing Functions

| Name | Variable | Curve | Feel |
|------|----------|-------|------|
| Bounce | `--spring-bounce` | `cubic-bezier(0.34, 1.56, 0.64, 1)` | Overshoot, playful. |
| Smooth | `--spring-smooth` | `cubic-bezier(0.22, 1, 0.36, 1)` | Natural, organic. |
| Snappy | `--spring-snappy` | `cubic-bezier(0.16, 1, 0.3, 1)` | Quick, responsive. |
| Gentle | `--spring-gentle` | `cubic-bezier(0.33, 1, 0.68, 1)` | Calm, deliberate. |

### Duration Scale

| Speed | Variable | Time | Usage |
|-------|----------|------|-------|
| Instant | `--duration-instant` | 100ms | Press feedback. |
| Fast | `--duration-fast` | 150ms | Hover states. |
| Normal | `--duration-normal` | 250ms | Standard transitions. |
| Slow | `--duration-slow` | 400ms | Reveals, entrances. |
| Dramatic | `--duration-dramatic` | 600ms | Major page changes. |

### Composite Transitions

```css
--transition-micro: 100ms cubic-bezier(0.16, 1, 0.3, 1);    /* Clicks */
--transition-fast: 150ms cubic-bezier(0.22, 1, 0.36, 1);   /* Hovers */
--transition-normal: 250ms cubic-bezier(0.22, 1, 0.36, 1); /* General */
--transition-bounce: 250ms cubic-bezier(0.34, 1.56, 0.64, 1); /* Fun stuff */
```

---

## 4. Shadow & Glow System

### Shadows (Depth)

| Size | Variable | Value |
|------|----------|-------|
| Small | `--shadow-sm` | `0 1px 2px rgba(0,0,0,0.5)` |
| Medium | `--shadow-md` | `0 4px 6px rgba(0,0,0,0.4), 0 2px 4px rgba(0,0,0,0.3)` |
| Large | `--shadow-lg` | `0 10px 15px rgba(0,0,0,0.4), 0 4px 6px rgba(0,0,0,0.3)` |
| XL | `--shadow-xl` | `0 20px 25px rgba(0,0,0,0.5), 0 8px 10px rgba(0,0,0,0.3)` |

### Glows (Light)

| Intensity | Variable | Value |
|-----------|----------|-------|
| Subtle | `--glow-subtle` | `0 0 20px rgba(255,255,255,0.08)` |
| Medium | `--glow-medium` | `0 0 30px rgba(255,255,255,0.15)` |
| Strong | `--glow-strong` | `0 0 40px rgba(255,255,255,0.25)` |
| Selection | `--glow-selection` | `0 0 20px rgba(255,255,255,0.12), 0 0 40px rgba(255,255,255,0.06)` |

---

## 5. Component Architecture

### The Wallpaper Card (Liquid Window)

The card is the hero. It must feel like a portal to another world.

**States:**
- **Rest:** Obsidian background, subtle border, small shadow.
- **Hover:** Lifts (translateY -4px), scales 1.02, border brightens, glow appears. Image zooms 1.08.
- **Active/Press:** Sinks (translateY -2px, scale 0.98), shadow reduces. Instant feedback.
- **Selected:** Luminous border (2px white), strong glow.

**Elements:**
- **Title Overlay:** Gradient fade, slides up on hover.
- **Favorite Button:** Hidden at rest, scales in on hover, pulses when active.
- **Loading Skeleton:** Shimmer animation.

```css
.wallpaper-card:hover {
    transform: translateY(-4px) scale(1.02);
    box-shadow: var(--shadow-lg), var(--glow-medium);
}

.wallpaper-card:active {
    transform: translateY(-2px) scale(0.98);
    transition: transform 100ms cubic-bezier(0.16, 1, 0.3, 1);
}
```

### The Sidebar (Glass Dock)

**Material:** Carbon base with subtle border.

**Item States:**
- **Rest:** Transparent.
- **Hover:** Elevated background, slides right 2px.
- **Active:** Slides right 4px, scales down 0.98.
- **Selected:** Glass background, visible border, glow, white text.

**Section Headers:** Ultra-small (10px), wide tracking (0.15em), ghost color.

### Buttons (Tactile Response)

**Primary (`.suggested-action`):**
- White background, void text.
- Hover: Pure white, stronger glow.
- Active: Slight gray, no glow, scale 0.95.

**Secondary (Default):**
- Carbon elevated background, subtle border.
- Hover: Brighter background, visible border, subtle glow.
- Active: Scale 0.95, no glow.

**Flat (`.flat`):**
- Transparent until hover.
- Hover: Light white tint.

### Glass Panels (Floating Surfaces)

For modals, popovers, and floating UI.

```css
.glass-panel {
    background-color: rgba(10, 10, 10, 0.85);
    border-radius: 18px;
    border: 1px solid rgba(255, 255, 255, 0.10);
    box-shadow: 
        0 20px 25px rgba(0, 0, 0, 0.5),
        0 0 20px rgba(255, 255, 255, 0.08),
        inset 0 1px 0 rgba(255, 255, 255, 0.05);
}
```

---

## 6. Micro-Interactions

### Click Feedback
- Buttons: Scale to 0.95 on press.
- Cards: Scale to 0.98, lift reduces.
- Transition: 100ms with snappy spring.

### Hover Feedback
- Elements "lift" towards the cursor.
- Borders brighten.
- Subtle glow appears.
- Icons increase opacity.

### Cascade Entrance
- List items stagger in with vertical offset.
- Animation: `translateY(10px) → 0`, `opacity: 0 → 1`.
- Delay: 30ms per item.

### Skeleton Loading
- Gradient shimmer sweep.
- Direction: Left to right.
- Duration: 1.5s, infinite.

---

## 7. Spacing & Radii

### Spacing Scale (4px base)

| Token | Value |
|-------|-------|
| `--space-1` | 4px |
| `--space-2` | 8px |
| `--space-3` | 12px |
| `--space-4` | 16px |
| `--space-5` | 20px |
| `--space-6` | 24px |
| `--space-8` | 32px |
| `--space-10` | 40px |
| `--space-12` | 48px |

### Border Radii

| Token | Value | Usage |
|-------|-------|-------|
| `--radius-sm` | 6px | Badges, small buttons. |
| `--radius-md` | 10px | Buttons, inputs. |
| `--radius-lg` | 14px | Cards, panels. |
| `--radius-xl` | 18px | Modals, large panels. |
| `--radius-2xl` | 24px | Hero elements. |
| `--radius-full` | 9999px | Pills, circles. |

---

## 8. Implementation Notes

### CSS Variables
All tokens are defined in `:root` in `data/ui/style.css`.

### GTK4 Limitations
- `backdrop-filter` has limited support. We simulate blur with semi-transparent surfaces.
- Complex animations may need to be applied via widget properties in C++.

### CSS Hot-Reload
The application supports live CSS reloading. Edit `style.css` and save to see changes instantly.

### Typography
Font stack: `Geist → Inter → SF Pro Text → system`.
Ensure Geist is installed for optimal appearance.

---

## 9. Status Colors (Semantic)

| Status | Background | Border | Text |
|--------|------------|--------|------|
| Error | `rgba(127,29,29,0.4)` | `rgba(239,68,68,0.3)` | `#FCA5A5` |
| Success | `rgba(6,78,59,0.4)` | `rgba(34,197,94,0.3)` | `#86EFAC` |
| Warning | `rgba(120,53,15,0.4)` | `rgba(245,158,11,0.3)` | `#FCD34D` |
| Steam | `rgba(23,32,38,0.6)` | `rgba(49,63,74,0.5)` | — |

---

## 10. The Rules

1. **No Pure Black (#000000)** — Use `--void` (#0A0A0A).
2. **No Pure White (#FFFFFF)** — Use `--text-primary` (#F0F0F0).
3. **No Color in UI** — Only content and status.
4. **Every Hover Animates** — Nothing is dead.
5. **Every Click Responds** — Tactile or fail.
6. **Springs, Not Linear** — Organic motion only.
7. **Respect the Blur** — Floating elements are glass.

---

*End of Liquid Glass Design System v2.0*
