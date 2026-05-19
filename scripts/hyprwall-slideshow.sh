#!/usr/bin/env bash
# hyprwall-slideshow.sh
# Usage: hyprwall-slideshow.sh <monitor> <folder> <interval_seconds>
# Picks a random image file each tick and applies it via hyprctl hyprpaper IPC.
# Runs as a daemon (background loop). Only one instance per monitor is allowed.

MONITOR="$1"
FOLDER="$2"
INTERVAL="${3:-600}"

if [[ -z "$MONITOR" || -z "$FOLDER" ]]; then
    echo "Usage: hyprwall-slideshow.sh <monitor> <folder> [interval_seconds]" >&2
    exit 1
fi

# Image extensions
EXTS=(jpg jpeg png bmp gif webp)

# Build find -name expression
NAME_ARGS=()
first=1
for ext in "${EXTS[@]}"; do
    if [[ $first -eq 1 ]]; then
        NAME_ARGS+=( -iname "*.${ext}" )
        first=0
    else
        NAME_ARGS+=( -o -iname "*.${ext}" )
    fi
done

apply_random() {
    # Collect all image files
    mapfile -t FILES < <(find "$FOLDER" -maxdepth 1 -type f \( "${NAME_ARGS[@]}" \))
    local count=${#FILES[@]}
    if [[ $count -eq 0 ]]; then
        echo "[hyprwall-slideshow] No images found in $FOLDER" >&2
        return 1
    fi

    # Pick random index
    local idx=$(( RANDOM % count ))
    local img="${FILES[$idx]}"

    echo "[hyprwall-slideshow] Applying: $img -> $MONITOR"

    # Preload
    local preload_result
    preload_result=$(hyprctl hyprpaper preload "$img" 2>&1)
    echo "[hyprwall-slideshow] preload: $preload_result"

    # Apply wallpaper (cover mode)
    local wp_result
    wp_result=$(hyprctl hyprpaper wallpaper "${MONITOR},${img}" 2>&1)
    echo "[hyprwall-slideshow] wallpaper: $wp_result"

    # Unload previously loaded images to avoid VRAM leak (keep last 2)
    # hyprpaper IPC: listloaded
    local loaded
    loaded=$(hyprctl hyprpaper listloaded 2>/dev/null)
    while IFS= read -r loaded_img; do
        [[ "$loaded_img" == "$img" ]] && continue
        hyprctl hyprpaper unload "$loaded_img" >/dev/null 2>&1
    done <<< "$loaded"

    return 0
}

# Apply immediately on start
apply_random

# Then loop
while true; do
    sleep "$INTERVAL"
    apply_random
done
