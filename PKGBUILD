# Maintainer: 7Ji <pugokushin at gmail dot com>
# Maintainer: leaeasy <leaeasy at gmail dot com>
# Maintainer: devome <evinedeng at hotmail dot com>

_pkgname=wechat-universal
pkgname=${_pkgname}-bwrap
pkgver=1.0.0.242
pkgrel=2
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
if [[ "${CARCH}" == loong64 ]]; then # This is needed instead of a plain declaration because AUR web would return all depends regardless of client's arch, AUR helpers would thus refuse to build the package on non-loong64 due to missing liblol
    depends_loong64=('liblol')
fi
options=(!strip !debug emptydirs) # emptydirs for /usr/lib/license (see below)

_lib_uos='libuosdevicea'

source=(
    "fake_dde-file-manager"
    "${_pkgname}.sh"
    "${_pkgname}.desktop"
    "${_lib_uos}".{c,Makefile}
)

_rpm_stem="wechat-beta_${pkgver}"
_rpm_url_common_1="https://mirrors.opencloudos.tech/opencloudos/9.2/extras/"
_rpm_url_common_2="/os/Packages/${_rpm_stem}"
_deb_stem="com.tencent.wechat_1.0.0.241"
_deb_url_common="https://home-store-packages.uniontech.com/appstore/pool/appstore/c/com.tencent.wechat/${_deb_stem}"

source_x86_64=("${_rpm_url_common_1}x86_64${_rpm_url_common_2}_amd64.rpm")
source_aarch64=("${_rpm_url_common_1}aarch64${_rpm_url_common_2}_arm64.rpm")
source_loong64=("${_deb_url_common}_loongarch64.deb")

noextract=("${_rpm_stem}"_{amd,arm}64.rpm "${_deb_stem}"_loongarch64.deb )

sha256sums=(
    'b25598b64964e4a38f8027b9e8b9a412c6c8d438a64f862d1b72550ac8c75164'
    '659485bdee618cf58809f8d022d8238231656b2a0c590742f4527b0f81f0fd19'
    'b783b7b0035efb5a0fcb4ddba6446f645a4911e4a9f71475e408a5c87ef04c30'
    'fc3ce9eb8dee3ee149233ebdb844d3733b2b2a8664422d068cf39b7fb08138f8'
    'f05f6f907898740dab9833c1762e56dbc521db3c612dd86d2e2cd4b81eb257bf'
)

sha256sums_x86_64=(
    'ff97d711f3c71cbe86ef93e1d04a681af5c3e95e0188f4b411064ced2819d719'
)
sha256sums_aarch64=(
    '8961d5a61e3006438d140accd88a04f72796cc1c147e048b1751e03e5ce4f4ed'
)
sha256sums_loong64=(
    '90c3276fd8e338eb50162bcb0eef9a41cb553187851d0d5f360e3d010138c8b9'
)

prepare() {
    local _arch=
    case "${CARCH}" in
        loong64)
            _arch=loongarch64
            ;;
        *)
    esac
    if [[ "${_arch}" ]]; then
        echo 'Extracting data.tar from deb...'
        bsdtar -xOf "${_deb_stem}_${_arch}.deb" ./data.tar.xz |
            xz -cd > data.tar
    fi
    echo 'Preparing to compile libuosdevica.so...'
    mkdir -p "${_lib_uos}"
    mv "${_lib_uos}"{.c,/}
    mv "${_lib_uos}"{.Makefile,/Makefile}
}

build() {
    cd "${_lib_uos}"
    echo "Building ${_lib_uos}.so stub by Zephyr Lykos..."
    make
}

package() {
    local _arch=
    case "${CARCH}" in
        x86_64)
            _arch=amd64
            ;;
        aarch64)
            _arch=arm64
            ;;
    esac
    if [[ "${_arch}" ]]; then
        echo 'Popupating pkgdir with data from wechat-universal rpm file...'
        bsdtar -C "${pkgdir}" --no-same-owner -xf "${_rpm_stem}_${_arch}.rpm" ./opt/wechat-beta
        install -DTm644 "${pkgdir}"/opt/wechat-beta/icons/wechat.png "${pkgdir}/usr/share/icons/hicolor/256x256/apps/${_pkgname}.png"
        rm -rf "${pkgdir}/opt/wechat-beta/icons"
        mv "${pkgdir}"/opt/{wechat-beta,"${_pkgname}"}
    else
        echo 'Popupating pkgdir with data from wechat-universal deb file...'
        tar -C "${pkgdir}" --no-same-owner -xf data.tar ./opt/apps/com.tencent.wechat
        mv "${pkgdir}"/opt/{apps/com.tencent.wechat/files,"${_pkgname}"}
        rm "${pkgdir}/opt/${_pkgname}/${_lib_uos}.so"

        echo 'Installing icons...'
        for res in 16 32 48 64 128 256; do
            install -Dm644 \
                "${pkgdir}/opt/apps/com.tencent.wechat/entries/icons/hicolor/${res}x${res}/apps/com.tencent.wechat.png" \
                "${pkgdir}/usr/share/icons/hicolor/${res}x${res}/apps/${_pkgname}.png"
        done
        rm -rf "${pkgdir}"/opt/apps
    fi

    echo 'Fixing licenses...'
    local _wechat_root="${pkgdir}/usr/share/${_pkgname}"
    install -dm755 "${pkgdir}"/usr/lib/license # This is needed if /usr/lib/license/${_lib_uos}.so needs to be mounted in sandbox
    install -Dm755 {"${_lib_uos}","${_wechat_root}"/usr/lib/license}"/${_lib_uos}.so"
    echo 'DISTRIB_ID=uos' |
        install -Dm755 /dev/stdin "${_wechat_root}"/etc/lsb-release

    echo 'Installing fake deepin file manager...'
    install -Dm755 {fake_,"${_wechat_root}"/usr/bin/}dde-file-manager

    echo 'Installing desktop files...'
    install -Dm644 "${_pkgname}.desktop" "${pkgdir}/usr/share/applications/${_pkgname}.desktop"
    install -Dm755 "${_pkgname}.sh" "${pkgdir}/usr/bin/${_pkgname}"
}
