# HyprWall

**GUI-менеджер обоев для Hyprland** с поддержкой видео-обоев и гибкими настройками на каждый монитор.

## Возможности

- 🖥️ Визуальный обзор всех мониторов с их расположением и текущими обоями
- 🖼️ Поддержка изображений (jpg, png, bmp, gif) и видео (mp4, mkv, webm, avi...)
- 🔊 Управление звуком видео-обоев + команда для биндинга в hyprland.conf
- 📐 13 режимов заполнения (fill, contain, stretch, center, tile и др.)
- 🔄 4 варианта поворота обоев
- 💾 Автосохранение настроек + systemd user-сервис для автовосстановления

## Зависимости

- Qt6 (Widgets, Gui, Core)
- [hyprpaper](https://github.com/hyprwm/hyprpaper) — для статических обоев
- [mpvpaper](https://github.com/GhostNaN/mpvpaper) — для видео-обоев

## Сборка

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

## Установка сервиса автозапуска

```bash
systemctl --user enable --now hyprwall
```

## CLI

```bash
# Открыть GUI
hyprwall

# Переключить звук видео-обоев (для биндинга в hyprland.conf)
hyprwall --toggle-audio DP-1

# Восстановить обои (вызывается сервисом)
hyprwall --daemon
```

### Пример биндинга в hyprland.conf

```conf
bind = , F9, exec, hyprwall --toggle-audio DP-1
```

## Установка через PKGBUILD (Arch)

```bash
makepkg -si
```
