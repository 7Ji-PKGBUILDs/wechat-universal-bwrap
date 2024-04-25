# Maintainer: 7Ji <pugokushin at gmail dot com>
# Maintainer: leaeasy <leaeasy at gmail dot com>
# Maintainer: devome <evinedeng at hotmail dot com>

_pkgname=wechat-universal
pkgname=${_pkgname}-bwrap
pkgver=1.0.0.241
pkgrel=1
pkgdesc="WeChat (Universal) with bwrap sandbox"
arch=('x86_64' 'aarch64' 'loong64')
url="https://weixin.qq.com"
license=('proprietary' 'GPLv3') # GPLv3 as desktop-app was statically linked-in, refer: https://aur.archlinux.org/packages/wechat-universal-bwrap#comment-964013
provides=("${_pkgname}")
conflicts=("${_pkgname}"{,-privileged})
replaces=('wechat-beta'{,-bwrap})
install="${_pkgname}.install"
depends=(
    'alsa-lib'
    'at-spi2-core'
    'bubblewrap'
    'flatpak-xdg-utils'
    'libxcomposite'
    'libxkbcommon-x11'
    'libxrandr'
    'mesa'
    'nss'
    'pango'
    'xcb-util-image'
    'xcb-util-keysyms'
    'xcb-util-renderutil'
    'xcb-util-wm'
    'xdg-desktop-portal'
    'xdg-user-dirs'
)
options=(!strip !debug emptydirs) # emptydirs for /usr/lib/license (see below)

_lib_uos='libuosdevicea'

source=(
    "fake_dde-file-manager"
    "${_pkgname}.sh"
    "${_pkgname}.desktop"
    "${_lib_uos}.c"
)

_deb_url_common="https://home-store-packages.uniontech.com/appstore/pool/appstore/c/com.tencent.wechat/com.tencent.wechat_${pkgver}"
_deb_stem="${_pkgname}_${pkgver}"

source_x86_64=(
    "${_deb_stem}_x86_64.deb::${_deb_url_common}_amd64.deb"
)

source_aarch64=(
    "${_deb_stem}_aarch64.deb::${_deb_url_common}_arm64.deb"
)

source_loong64=(
    "${_deb_stem}_loong64.deb::${_deb_url_common}_loongarch64.deb"
)

noextract=("${_deb_stem}"_{x86_64,aarch64,loong64}.deb)

sha256sums=(
    'b25598b64964e4a38f8027b9e8b9a412c6c8d438a64f862d1b72550ac8c75164'
    'f223c7271fba3829b06ceee14912063ad05d8ea286940006bce1b7b0e4fd48c1'
    'b783b7b0035efb5a0fcb4ddba6446f645a4911e4a9f71475e408a5c87ef04c30'
    'a8dd9bbb41968fc023a27467bccf45c9a7264dbf0b6cf85c3533b02c81f3fa03'
)

sha256sums_x86_64=(
    '2768a97376f2073bd515ef81d64fa7b20668fa6c11a4177a2ed17fc9b03398a3'
)
sha256sums_aarch64=(
    'e4a0387a4855757a229ffed586e31b1bc5c8971d30eef1c079d6757e704e058a'
)
sha256sums_loong64=(
    '90c3276fd8e338eb50162bcb0eef9a41cb553187851d0d5f360e3d010138c8b9'
)

build() {
    echo "Building ${_lib_uos}.so stub by Zephyr Lykos..."
    gcc -fPIC -shared "${_lib_uos}.c" -o "${_lib_uos}.so"
    strip "${_lib_uos}.so"
}

package() {
    echo 'Popupating pkgdir with data from wechat-universal deb file...'
    bsdtar -xOf "${_deb_stem}_${CARCH}.deb" ./data.tar.xz |
        xz -cdT0 |
        bsdtar -xpC "${pkgdir}" ./opt/apps/com.tencent.wechat
    mv "${pkgdir}"/opt/{apps/com.tencent.wechat/files,"${_pkgname}"}
    rm "${pkgdir}/opt/${_pkgname}/${_lib_uos}.so"

    echo 'Installing icons...'
    for res in 16 32 48 64 128 256; do
        install -Dm644 \
            "${pkgdir}/opt/apps/com.tencent.wechat/entries/icons/hicolor/${res}x${res}/apps/com.tencent.wechat.png" \
            "${pkgdir}/usr/share/icons/hicolor/${res}x${res}/apps/${_pkgname}.png"
    done
    rm -rf "${pkgdir}"/opt/apps

    echo 'Fixing licenses...'
    local _wechat_root="${pkgdir}/usr/share/${_pkgname}"
    install -dm755 "${pkgdir}"/usr/lib/license # This is needed if /usr/lib/license/${_lib_uos}.so needs to be mounted in sandbox
    install -Dm755 {,"${_wechat_root}"/usr/lib/license/}"${_lib_uos}.so"
    echo 'DISTRIB_ID=uos' |
        install -Dm755 /dev/stdin "${_wechat_root}"/etc/lsb-release

    echo 'Installing fake deepin file manager...'
    install -Dm755 {fake_,"${_wechat_root}"/usr/bin/}dde-file-manager

    echo 'Installing desktop files...'
    install -Dm644 "${_pkgname}.desktop" "${pkgdir}/usr/share/applications/${_pkgname}.desktop"
    install -Dm755 "${_pkgname}.sh" "${pkgdir}/usr/bin/${_pkgname}"
}
