# Maintainer: 7Ji <pugokushin at gmail dot com>
# Maintainer: leaeasy <leaeasy at gmail dot com>
# Maintainer: devome <evinedeng at hotmail dot com>

_pkgname=wechat-universal
pkgname=${_pkgname}-bwrap
pkgver=4.0.0.30
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
options=(!strip !debug)

source=(
    "${_pkgname}.sh"
    "${_pkgname}.desktop"
)

_upstream_name='wechat'
_deb_url_common='https://dldir1v6.qq.com/weixin/Universal/Linux/WeChatLinux_'
_deb_prefix="${_pkgname}-${pkgver}-"

source_x86_64=("${_deb_prefix}x86_64.deb::${_deb_url_common}x86_64.deb")
source_aarch64=("${_deb_prefix}aarch64.deb::${_deb_url_common}arm64.deb")
source_loong64=("${_deb_prefix}loong64.deb::${_deb_url_common}LoongArch.deb")

noextract=("${_deb_prefix}"{x86_64,aarch64,loong64}.deb )

sha256sums=(
    'cbaf57f763c12a05aa42093d8bdb5931a70972c3b252759ad9091e0d3350ebd1'
    '0563472cf2c74710d1fe999d397155f560d3ed817e04fd9c35077ccb648e1880'
)

sha256sums_x86_64=(
    '0e2834310e1d321da841a4cde9c657d9c086c5ee8c82d09e47eab53629a5038f'
)
sha256sums_aarch64=(
    'dd1f5029bb20274dd7903a0c5bf7796e1392262e048ce88265530a88328d72ec'
)
sha256sums_loong64=(
    '5017543b0fe8ad28bf862deec92c4a7faefa7b95bba205f20107a15c6a5a2098'
)

prepare() {
    echo 'Extracting data from deb...'
    bsdtar --extract --to-stdout --file "${_deb_prefix}${CARCH}.deb" ./data.tar.xz |
        xz --to-stdout --decompress --threads 0 |
        tar --strip-components 2 --extract ./opt/"${_upstream_name}" ./usr/share/icons/hicolor

    mv share/icons/hicolor icons
    rm -rf share "${_upstream_name}"/libuosdevicea.so

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

    echo 'Installing scripts...'
    install -Dm755 wechat-universal.sh "${_wechat_root}"/common.sh
    ln -s common.sh "${_wechat_root}"/start.sh
    ln -s common.sh "${_wechat_root}"/stop.sh
    mkdir -p "${pkgdir}"/usr/bin
    ln -s ../lib/"${_pkgname}"/common.sh "${pkgdir}"/usr/bin/"${_pkgname}"

    echo 'Installing desktop files...'
    install -Dm644 "${_pkgname}.desktop" "${pkgdir}/usr/share/applications/${_pkgname}.desktop"
}
