# Maintainer: 7Ji <pugokushin at gmail dot com>
# Maintainer: leaeasy <leaeasy at gmail dot com>
# Maintainer: devome <evinedeng at hotmail dot com>

_pkgname=wechat-universal
pkgname=${_pkgname}-bwrap
pkgver=4.0.0.21
pkgrel=1
pkgdesc="WeChat (Universal) with bwrap sandbox"
arch=('x86_64' 'aarch64' 'loong64')
url="https://weixin.qq.com"
license=('proprietary' 'GPLv3') # GPLv3 as desktop-app was statically linked-in, refer: https://aur.archlinux.org/packages/wechat-universal-bwrap#comment-964013
install="${_pkgname}".install
provides=("${_pkgname}")
conflicts=("${_pkgname}")
replaces=('wechat-beta'{,-bwrap})
depends=('at-spi2-core' 'bubblewrap' 'flatpak-xdg-utils' 'jack' 'libpulse' 'libxcomposite' 'libxdamage' 'libxkbcommon-x11' 'libxrandr'
         'mesa' 'nss' 'pango' 'xcb-util-image' 'xcb-util-keysyms' 'xcb-util-renderutil' 'xcb-util-wm' 'xdg-desktop-portal' 'xdg-user-dirs')
makedepends=("patchelf")
if [[ "${CARCH}" == loong64 ]]; then # This is needed instead of a plain declaration because AUR web would return all depends regardless of client's arch, AUR helpers would thus refuse to build the package on non-loong64 due to missing liblol
    depends_loong64=('liblol')
fi

_lib_uos='libuosdevicea'
_deb_stem="com.tencent.wechat_${pkgver}"
_deb_url_common="https://home-store-packages.uniontech.com/appstore/pool/appstore/c/com.tencent.wechat/${_deb_stem}"

source=('fake_dde-file-manager'
        "${_pkgname}.sh"
        "${_pkgname}.desktop"
        "${_lib_uos}".{c,Makefile})
source_x86_64=("${_pkgname}-${pkgver}-x86_64.deb::${_deb_url_common}_amd64.deb")
source_aarch64=("${_pkgname}-${pkgver}-aarch64.deb::${_deb_url_common}_arm64.deb")
source_loong64=("${_pkgname}-${pkgver}-loong64.deb::${_deb_url_common}_loongarch64.deb")
sha256sums=('b25598b64964e4a38f8027b9e8b9a412c6c8d438a64f862d1b72550ac8c75164'
            'd5ab68ccc7e86fb24452a96c5e5e98c3bea28edb3a1e5f3d495b2660ac2dba89'
            '0563472cf2c74710d1fe999d397155f560d3ed817e04fd9c35077ccb648e1880'
            'fc3ce9eb8dee3ee149233ebdb844d3733b2b2a8664422d068cf39b7fb08138f8'
            'f05f6f907898740dab9833c1762e56dbc521db3c612dd86d2e2cd4b81eb257bf')
sha256sums_x86_64=('d6d3bc011b762ee0b03f3eeb3b6e7ff5d4ce21ff51c2a73d4ddaa26534e88d19')
sha256sums_aarch64=('5e5f9b4ff597679f2b721bd404031f71da8cc4e211c69e6ac8c444fc5bc40bd2')
sha256sums_loong64=('6fa8f7cb5d0739d46f2a84d363d8f68c6a0cfd6f57023748ad035903d75d994c')
options=("!strip" "!debug" "emptydirs") # emptydirs for /usr/lib/license (see below)

prepare() {
    echo '    Extracting data.tar...'
    tar -xf data.tar.xz
    mv "opt/apps/com.tencent.wechat/files"         "${_pkgname}"
    mv "opt/apps/com.tencent.wechat/entries/icons" icons
    rm "${_pkgname}/${_lib_uos}.so"
    find "${_pkgname}" -type f | while read file; do
        if [[ $(file "$file") != *"ELF "* ]]; then
            chmod 644 "$file"
        fi
    done

    echo '    Preparing to compile libuosdevica.so...'
    mkdir -p "${_lib_uos}"
    mv "${_lib_uos}"{.c,/}
    mv "${_lib_uos}"{.Makefile,/Makefile}
}

build() {
    patchelf --set-rpath '$ORIGIN' "${_pkgname}/libwxtrans.so"
    find "${_pkgname}/vlc_plugins" -type f | xargs -I {} patchelf --set-rpath '$ORIGIN:$ORIGIN/../..' {}

    cd "${_lib_uos}"
    echo "    Building ${_lib_uos}.so stub by Zephyr Lykos..."
    make
}

package() {
    echo "    Installing ${_pkgname}..."
    for res in 16 32 48 64 128 256; do
        install -Dm644 "icons/hicolor/${res}x${res}/apps/com.tencent.wechat.png" "${pkgdir}/usr/share/icons/hicolor/${res}x${res}/apps/${_pkgname}.png"
    done
    install -dm755 "${pkgdir}/opt"
    cp -r --preserve=mode "${_pkgname}"  "${pkgdir}/opt/${_pkgname}"

    echo '    Fixing licenses...'
    local _wechat_root="${pkgdir}/usr/lib/${_pkgname}"
    install -dm755 "${pkgdir}"/usr/lib/license # This is needed if /usr/lib/license/${_lib_uos}.so needs to be mounted in sandbox
    install -Dm755 {"${_lib_uos}","${_wechat_root}"/usr/lib/license}"/${_lib_uos}.so"
    echo 'DISTRIB_ID=uos' |
        install -Dm755 /dev/stdin "${_wechat_root}"/etc/lsb-release

    echo '    Installing scripts...'
    install -Dm755 wechat-universal.sh "${_wechat_root}"/common.sh
    ln -s common.sh "${_wechat_root}"/start.sh
    ln -s common.sh "${_wechat_root}"/stop.sh

    echo '    Installing fake deepin file manager...'
    install -Dm755 {fake_,"${_wechat_root}"/usr/bin/}dde-file-manager

    echo '    Installing desktop files...'
    install -Dm644 "${_pkgname}.desktop" "${pkgdir}/usr/share/applications/${_pkgname}.desktop"
    printf '%s\n' '#!/bin/bash' 'exec /usr/lib/'"${_pkgname}"'/start.sh "$@"' | install -DTm 755 /dev/stdin "${pkgdir}"/usr/bin/"${_pkgname}"
}
