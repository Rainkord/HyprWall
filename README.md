<p align="center">
  <img src="assets/screenshot.png" width="800" alt="HyprWall Screenshot">
</p>

<h1 align="center">HyprWall</h1>

<p align="center">
  A graphical wallpaper manager for <a href="https://hyprland.org">Hyprland</a><br>
  Static, video, and GIF wallpapers with lock screen sync and slideshow support
</p>

<p align="center">
  <a href="https://github.com/Rainkord/HyprWall/blob/main/LICENSE"><img src="https://img.shields.io/github/license/Rainkord/HyprWall?color=blue" alt="License"></a>
  <a href="https://aur.archlinux.org/packages/hyprwall-git"><img src="https://img.shields.io/aur/version/hyprwall-git?label=AUR" alt="AUR"></a>
  <a href="https://github.com/Rainkord/HyprWall"><img src="https://img.shields.io/github/stars/Rainkord/HyprWall?style=social" alt="Stars"></a>
</p>

---

## Features

- **Per-monitor wallpapers** — set different images on each display
- **Video & GIF wallpapers** — animated backgrounds via mpvpaper
- **Slideshow mode** — auto-rotate wallpapers with custom interval
- **Lock screen sync** — manage hyprlock wallpapers from the same app
- **Same wallpaper toggle** — instantly sync desktop and lock screen backgrounds
- **Gallery** — browse and manage your wallpaper collection with instant thumbnails
- **Multilingual** — English and Russian localization (persists on restart)
- **Autostart** — launch on login with a single toggle

## Requirements

| Dependency | Purpose |
|---|---|
| `qt6-base` | UI framework |
| `wayland` | Display protocol |
| `hyprpaper` | Static image wallpapers |
| `mpvpaper` | Video & GIF wallpapers |
| `hyprlock` | Lock screen management |
| `papirus-icon-theme` | Icons (optional) |

## Installation

### AUR (recommended)

```bash
yay -S hyprwall-git
```

### Manual build

```bash
git clone https://github.com/Rainkord/HyprWall.git
cd HyprWall
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --install build
```

## Usage

```bash
hyprwall
```

1. Select a monitor from the top bar
2. Pick a wallpaper from the gallery or add your own
3. Configure orientation, slideshow, and audio settings
4. Enable **Same wallpaper** to sync with lock screen

### Keyboard shortcuts

| Key | Action |
|---|---|
| `SUPER + L` | Lock screen |

### Autostart

Enable the autostart toggle in the app settings. This creates
`~/.config/autostart/hyprwall.desktop` which launches `hyprwall --daemon` on login.

## Updating

```bash
cd HyprWall
git pull
cmake --build build -j$(nproc)
sudo cmake --install build
```

## Contributing

Pull requests are welcome. For major changes, please open an issue first.

## License

[MIT](LICENSE)
