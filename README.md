# HyprWall

Wallpaper manager for Hyprland.

## Build & Install

```bash
git clone https://github.com/Rainkord/HyprWall.git
cd HyprWall
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

After install, launch from anywhere:

```bash
hyprwall
```

## Update

```bash
cd HyprWall
git pull
cmake --build build -j$(nproc)
sudo cmake --install build
```

## Dependencies

- Qt6 (Widgets, Gui, Core)
- wayland-client
- hyprpaper (for images)
- mpvpaper (for video)
