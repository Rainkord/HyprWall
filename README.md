# HyprWall

> A graphical wallpaper manager for Hyprland with image, video, and slideshow support.

![License](https://img.shields.io/github/license/Rainkord/HyprWall)
![AUR](https://img.shields.io/aur/version/hyprwall-git)

---

## Features

- 🖼️ Set static wallpapers per monitor
- 🎥 Video wallpapers via `mpvpaper`
- 🔄 Slideshow mode with configurable interval
- 🖥️ Multi-monitor support
- 💾 Autostart on login
- 🎨 Dark translucent UI designed for Hyprland

## Requirements

| Dependency | Purpose |
|---|---|
| `qt6-base` | UI framework |
| `wayland` | Display protocol |
| `hyprpaper` | Image wallpaper backend |
| `mpvpaper` | Video wallpaper backend |
| `papirus-icon-theme` *(optional)* | Icon theme support |

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

Select a monitor from the top bar, pick a wallpaper mode (image / video / slideshow), and apply.

### Autostart

Enable autostart from the settings panel inside the app. This creates
`~/.config/autostart/hyprwall.desktop` which launches `hyprwall --daemon` on login.

## Updating (manual build)

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
