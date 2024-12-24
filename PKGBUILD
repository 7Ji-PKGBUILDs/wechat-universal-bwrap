# Maintainer: 7Ji <pugokushin at gmail dot com>
# Maintainer: leaeasy <leaeasy at gmail dot com>
# Maintainer: devome <evinedeng at hotmail dot com>

_pkgname=wechat-universal
pkgname=${_pkgname}-bwrap
pkgver=4.0.1.11
pkgrel=1
pkgdesc="WeChat (Universal) with bwrap sandbox"
arch=('x86_64' 'aarch64' 'loong64')
url='https://linux.weixin.qq.com/'
license=('proprietary' 'GPLv3') # GPLv3 as desktop-app was statically linked-in, refer: https://aur.archlinux.org/packages/wechat-universal-bwrap#comment-964013
install="${_pkgname}".install
provides=("${_pkgname}")
conflicts=("${_pkgname}")
replaces=('wechat-beta'{,-bwrap})
depends=(
    'at-spi2-core'
    'bubblewrap'
    'flatpak-xdg-utils'
    'jack'
    'libpulse'
    'libxcomposite'
    'libxdamage'
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
makedepends=(
    'patchelf'
)
if [[ "${CARCH}" == loong64 ]]; then # This is needed instead of a plain declaration because AUR web would return all depends regardless of client's arch, AUR helpers would thus refuse to build the package on non-loong64 due to missing liblol
    depends_loong64=('liblol')
fi
options=(!strip !debug emptydirs)

_lib_uos='libuosdevicea'

source=(
    "${_pkgname}.sh"
    "${_pkgname}.desktop"
    "${_lib_uos}".{c,Makefile}
)

_upstream_name='wechat'
_deb_url_common='https://dldir1v6.qq.com/weixin/Universal/Linux/WeChatLinux_'
_deb_prefix="${_pkgname}-${pkgver}-"

source_x86_64=("${_deb_prefix}x86_64.deb::${_deb_url_common}x86_64.deb")
source_aarch64=("${_deb_prefix}aarch64.deb::${_deb_url_common}arm64.deb")
source_loong64=("${_deb_prefix}loong64.deb::${_deb_url_common}LoongArch.deb")

noextract=("${_deb_prefix}"{x86_64,aarch64,loong64}.deb )

sha256sums=(
    'effef03854827bcd02b0476d33e62f9c7ef1f87f949e89847e1f052c6f09f841'
    '0563472cf2c74710d1fe999d397155f560d3ed817e04fd9c35077ccb648e1880'
    'fc3ce9eb8dee3ee149233ebdb844d3733b2b2a8664422d068cf39b7fb08138f8'
    'f05f6f907898740dab9833c1762e56dbc521db3c612dd86d2e2cd4b81eb257bf'
)

sha256sums_x86_64=(
    '16410e0ca7895e5aa375282de5e48b2d5fcb958b063de6b2dcfd02bed190cc01'
)
sha256sums_aarch64=(
    '59805ad7335de5ebb1fd5748d19b2f39761a085a45be30906810756d066277bc'
)
sha256sums_loong64=(
    'f6df1547bf93f6f0817ae4845227cd2917fda414f3255088bb6961e064c7b112'
)

prepare() {
    echo 'Extracting data from deb...'
    bsdtar --extract --to-stdout --file "${_deb_prefix}${CARCH}.deb" ./data.tar.xz |
        xz --to-stdout --decompress --threads 0 |
        tar --strip-components 2 --extract ./opt/"${_upstream_name}" ./usr/share/icons/hicolor

    mv share/icons/hicolor icons
    rm -rf share "${_upstream_name}"/libuosdevicea.so

    echo "Preparing to compile ${_lib_uos}.so..."
    mv "${_lib_uos}".Makefile Makefile

    echo 'Stripping executable permission of non-ELF files...'
    cd "${_upstream_name}"
    local _file
    find . -type f -perm /111 | while read _file; do
        if [[ $(file --brief "${_file}") == 'ELF '* ]]; then
            continue
        fi
        stat --printf '  %A => ' "${_file}"
        chmod u-x,g-x,o-x "${_file}"
        stat --format '%A %n' "${_file}"
    done

    echo 'Patching rpath...'
    patchelf --set-rpath '$ORIGIN' 'libwxtrans.so'
    echo "  '\$ORIGIN' <= libwxtrans.so"
    cd vlc_plugins
    find . -type f | while read _file; do
        echo "  '\$ORIGIN:\$ORIGIN/../..' <= vlc_plugins/${_file}"
        patchelf --set-rpath '$ORIGIN:$ORIGIN/../..' "${_file}"
    done
}

build() {
    echo "Building ${_lib_uos}.so stub by Zephyr Lykos..."
    make
}

package() {
    echo 'Popupating pkgdir with earlier extracted data...'
    mkdir -p "${pkgdir}"/opt
    cp -r --preserve=mode "${_upstream_name}" "${pkgdir}/opt/${_pkgname}"

    echo 'Installing icons...'
    for res in 16 32 48 64 128 256; do
        install -Dm644 \
            "icons/${res}x${res}/apps/${_upstream_name}.png" \
            "${pkgdir}/usr/share/icons/hicolor/${res}x${res}/apps/${_pkgname}.png"
    done

    local _wechat_root="${pkgdir}/usr/lib/${_pkgname}"

    echo 'Fixing licenses...'
    install -dm755 "${pkgdir}"/usr/lib/license # This is needed if /usr/lib/license/${_lib_uos}.so needs to be mounted in sandbox
    install -Dm755 {,"${_wechat_root}"/usr/lib/license/}"${_lib_uos}.so"
    echo 'DISTRIB_ID=uos' |
        install -Dm755 /dev/stdin "${_wechat_root}"/etc/lsb-release

    echo 'Installing scripts...'
    install -Dm755 wechat-universal.sh "${_wechat_root}"/common.sh
    ln -s common.sh "${_wechat_root}"/start.sh
    ln -s common.sh "${_wechat_root}"/stop.sh
    mkdir -p "${pkgdir}"/usr/bin
    ln -s ../lib/"${_pkgname}"/common.sh "${pkgdir}"/usr/bin/"${_pkgname}"

    echo 'Installing desktop files...'
    install -Dm644 "${_pkgname}.desktop" "${pkgdir}/usr/share/applications/${_pkgname}.desktop"
}
