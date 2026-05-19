# Maintainer: Rainkord
pkgname=hyprwall
pkgver=0.1.2
pkgrel=1
pkgdesc="GUI менеджер обоев для Hyprland с поддержкой видео"
arch=('x86_64')
url="https://github.com/Rainkord/HyprWall"
license=('MIT')
depends=('qt6-base' 'hyprpaper' 'mpvpaper' 'wayland')
optdepends=('papirus-icon-theme: icon theme support')
makedepends=('cmake' 'ninja' 'qt6-tools' 'wayland-protocols')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "HyprWall-$pkgver"
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    cd "HyprWall-$pkgver"
    DESTDIR="$pkgdir" cmake --install build
}
