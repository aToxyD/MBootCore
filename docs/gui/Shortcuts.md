# Keyboard Shortcuts

| Action | Shortcut | Context |
|--------|----------|---------|
| Quit | Ctrl+Q | Global |
| Discover Devices | F5 | Global |
| Settings | Ctrl+comma | Global |
| Show Discovery | Ctrl+1 | Global |
| Show Session | Ctrl+2 | Global |
| Show Firmware | Ctrl+3 | Global |
| Show Flash | Ctrl+4 | Global |
| Show GPT | Ctrl+5 | Global |
| Show Workflow | Ctrl+6 | Global |
| Show Jobs | Ctrl+7 | Global |
| Show Plugins | Ctrl+8 | Global |
| Show Vendors | Ctrl+9 | Global |
| Show Transport | Ctrl+0 | Global |

Shortcuts are managed by `ShortcutManager`, persisted in QSettings, and customizable via the Settings dialog. Each shortcut has a unique ID, default key, and current key. `restoreDefaults()` resets all.
