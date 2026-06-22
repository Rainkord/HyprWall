# PRD: Извлечение Q_OBJECT классов из MainWindow.cpp

## Problem Statement

HyprWall v0.2.1 — Qt6 менеджер обоев для Hyprland. Файл `MainWindow.cpp` (1337 строк) содержит три Q_OBJECT класса (ToggleSwitch, GalleryDelegate, MonitorBar) определённых прямо в .cpp файле. Это вынуждает использовать хак `#include "MainWindow.moc"` — нестандартный паттерн, который:

- Ломает навигацию IDE (cls/qmld не видят классы)
- Мешает тестированию (MonitorBar::computeMonitorRects() не вызвать извне)
- Блокирует переиспользование (ToggleSwitch и MonitorBar — generic компоненты)
- Создаёт хрупкую зависимость от порядка определения в .cpp файле
- Делает мерж-конфликты болезненнее при параллельной работе

## Solution

Извлечь три класса в отдельные .h/.cpp файлы с нативной AUTOMOC поддержкой. Удалить `#include "MainWindow.moc"`. Не трогать логику MainWindow — чистая структурная перестановка.

## User Stories

1. As a developer, I want ToggleSwitch to live in its own header so that I can include it in any Qt project without dragging in MainWindow
2. As a developer, I want MonitorBar to live in its own .h/.cpp so that I can unit-test computeMonitorRects() without instantiating a full QMainWindow
3. As a developer, I want GalleryDelegate to live in its own files so that I can understand its paint logic without scrolling past 1000 lines of unrelated code
4. As a developer, I want to delete `#include "MainWindow.moc"` so that the build system works with standard AUTOMOC
5. As a developer, I want gallery layout constants (THUMB_W, THUMB_H, GRID_PAD, LABEL_H) in a shared location so that GalleryDelegate and recalcGalleryLayout() use the same values
6. As a developer, I want each extracted widget to compile independently so that I can verify correctness incrementally
7. As a developer, I want MonitorBar to be a proper QObject so that its signal monitorClicked is MOC-processed natively
8. As a developer, I want the codebase to compile cleanly after extraction with zero behavioral changes
9. As a user, I want the UI to look and behave identically after refactoring — no visual changes, no interaction changes
10. As a user, I want ToggleSwitch animation to continue working smoothly after extraction
11. As a user, I want MonitorBar monitor selection to work identically after extraction
12. As a user, I want gallery thumbnails to render identically after extraction

## Implementation Decisions

### 1. Extract MonitorBar (first — biggest win, 129 lines)

- New files: `src/MonitorBar.h`, `src/MonitorBar.cpp`
- Contains: MonitorBar class, MonitorRect struct, computeMonitorRects()
- Dependencies: only `Types.h` (MonitorInfo struct) and standard Qt headers
- `monitorClicked(QString)` signal preserved exactly
- All public API preserved: `setMonitors()`, `setSelected()`, `setNoMonitorsText()`, `setMonitorMode()`

### 2. Extract ToggleSwitch (second — trivial, 48 lines)

- New file: `src/ToggleSwitch.h` (header-only, no .cpp)
- Contains: ToggleSwitch class with Q_PROPERTY动画
- Dependencies: only QWidget, QPainter, QPropertyAnimation
- `toggled(bool)` signal preserved
- No behavioral changes

### 3. Extract GalleryDelegate (third — needs constants)

- New files: `src/GalleryDelegate.h`, `src/GalleryDelegate.cpp`
- Contains: GalleryDelegate class, Q_OBJECT in header
- Gallery layout constants moved to `src/GalleryConstants.h`:
  ```
  THUMB_W=120, THUMB_H=68, GRID_PAD=6, LABEL_H=20
  ```
- Constants shared between GalleryDelegate and MainWindow::recalcGalleryLayout()
- paint() and sizeHint() signatures unchanged

### 4. Delete MainWindow.moc hack

- Remove `#include "MainWindow.moc"` from MainWindow.cpp
- Remove embedded class definitions (lines 48-192, 259-388)
- Remove static constants (lines 48-51) — moved to GalleryConstants.h
- Add #include for new headers

### 5. Update CMakeLists.txt

- Add to SOURCES: MonitorBar.cpp, GalleryDelegate.cpp
- Add to HEADERS: ToggleSwitch.h, MonitorBar.h, GalleryDelegate.h, GalleryConstants.h
- AUTOMOC already ON — no configuration changes needed

### 6. Update MainWindow.cpp includes

- Add: `#include "ToggleSwitch.h"`, `#include "MonitorBar.h"`, `#include "GalleryDelegate.h"`, `#include "GalleryConstants.h"`
- Remove: `#include <QStyledItemDelegate>`, `#include <QPropertyAnimation>`, `#include <QPainterPath>` (moved to extracted files)
- Remove embedded class definitions
- No logic changes — only structural moves

## Testing Decisions

- **Incremental compilation**: after each extraction, verify `cmake --build build` succeeds
- **Visual regression**: launch hyprwall, verify ToggleSwitch animation, MonitorBar painting, gallery thumbnails all render identically
- **No unit tests in this PRD**: test framework setup is a separate task (Chairman recommendation)
- **Smoke test**: every UI control (fill mode, rotation, audio, slideshow, gallery) must function identically
- **MonitorBar::computeMonitorRects()** becomes unit-testable after extraction — test harness deferred to next PRD

## Out of Scope

- Decomposing MainWindow into GalleryManager/SlideshowController/SettingsPanel (deferred)
- Decomposing buildUi() into sub-methods (deferred)
- Breaking WallpaperApplier ↔ ConfigManager circular dependency (deferred)
- Extracting autostart functions to AutostartManager (deferred)
- Unit test framework setup (separate task)
- Behavioral changes or new features
- CSS extraction (already done in v0.2.1)
- Atomic config save (already done in v0.2.1)

## Further Notes

- Total estimated time: ~2 hours (monitor 30min + toggle 15min + delegate 30min + cleanup 15min + verify 30min)
- Risk level: LOW — pure structural moves, no logic changes
- Reversibility: HIGH — git revert per extraction step
- The 3 extracted classes total ~260 lines, reducing MainWindow.cpp from 1337 to ~1077 lines
- MonitorBar extraction eliminates the only .Q_OBJECT class that has testable logic (computeMonitorRects)
- All cross-module signals (monitorClicked, toggled) are already using the correct Qt patterns
