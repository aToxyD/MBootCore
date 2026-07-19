# Theme System

## Architecture

`ThemeManager` manages four built-in themes (Dark, Light, Classic, High Contrast) with custom accent color support and font scale adjustment.

## How Themes Work

1. `ThemeManager::applyTheme(Theme)` sets the active theme
2. A QPalette is generated with theme-appropriate colors
3. `QApplication::setPalette()` applies the palette
4. A custom stylesheet is generated via `qApp->setStyleSheet()`
5. The `themeApplied` signal notifies all components

## Theme Colors

### Dark Theme
| Role | Color |
|------|-------|
| Window | #1e1e1e |
| WindowText | #d0d0d0 |
| Base | #2a2a2a |
| AlternateBase | #323232 |
| Text | #d0d0d0 |
| Button | #323232 |
| Highlight | Accent color |

### Light Theme
| Role | Color |
|------|-------|
| Window | #f0f0f0 |
| WindowText | #1e1e1e |
| Base | #ffffff |
| Text | #1e1e1e |
| Button | #f0f0f0 |

### High Contrast Theme
| Role | Color |
|------|-------|
| Window | #000000 |
| WindowText | #ffffff |
| Base | #000000 |
| Text | #ffffff |
| Link | #00ffff |

## Accent Color

Users can set a custom accent color for highlight, link text, progress bars, tab selection, and button hover/pressed states.

## Font Scale

The font scale adjusts the default application font size by a multiplier (1.0 = default, 1.2 = 20% larger, 0.8 = 20% smaller).

## Adding a New Theme

1. Add a value to the `Theme` enum in `ThemeManager.hpp`
2. Implement the palette generator method
3. Add colors to `themeStylesheet()`
4. Add the theme name to `availableThemes()` and `applyTheme(QString)`
5. Register in settings combo box
