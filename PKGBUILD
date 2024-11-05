# Maintainer: 7Ji <pugokushin at gmail dot com>
# Maintainer: leaeasy <leaeasy at gmail dot com>
# Maintainer: devome <evinedeng at hotmail dot com>

_pkgname=wechat-universal
pkgname=${_pkgname}-bwrap
pkgver=4.0.0.23
pkgrel=2
pkgdesc="WeChat (Universal) with bwrap sandbox"
arch=('x86_64' 'aarch64' 'loong64')
url="https://weixin.qq.com"
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

_deb_name='com.tencent.wechat'
_deb_url_common="https://home-store-packages.uniontech.com/appstore/pool/appstore/${_deb_name::1}/${_deb_name}/${_deb_name}_${pkgver}"
_deb_prefix="${_pkgname}-${pkgver}"

source_x86_64=("${_deb_prefix}-x86_64.deb::${_deb_url_common}_amd64.deb")
source_aarch64=("${_deb_prefix}-aarch64.deb::${_deb_url_common}_arm64.deb")
source_loong64=("${_deb_prefix}-loong64.deb::${_deb_url_common}_loongarch64.deb")

noextract=("${_deb_prefix}"_{x86,aarch,loong}64.deb )

sha256sums=(
    'cbaf57f763c12a05aa42093d8bdb5931a70972c3b252759ad9091e0d3350ebd1'
    '0563472cf2c74710d1fe999d397155f560d3ed817e04fd9c35077ccb648e1880'
)

sha256sums_x86_64=(
    '437826a3cdef25d763f69e29ae10479b5e8b2ba080b56de5b5de63e05a8f7203'
)
sha256sums_aarch64=(
    'a08b0f6c4930d7ecd7a73bd701511d9b29178dbe73ba51c04f09eb9e1b3190f7'
)
sha256sums_loong64=(
    '82b8fdc861d965a836d25e6cf0881c927bd4bf3d1f04791a9202ac39efab8662'
)

prepare() {
    echo 'Extracting data from deb...'
    bsdtar --extract --to-stdout --file "${_deb_prefix}-${CARCH}.deb" ./data.tar.xz |
        xz --to-stdout --decompress --threads 0 |
        tar --strip-components 4 --extract ./opt/apps/com.tencent.wechat/{entries/icons,files}

    mv entries/icons/hicolor icons
    rm -rf entries files/libuosdevicea.so

    echo 'Stripping executable permission of non-ELF files...'
    cd files
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
    cp -r --preserve=mode files "${pkgdir}/opt/${_pkgname}"

    echo 'Installing icons...'
    for res in 16 32 48 64 128 256; do
        install -Dm644 \
            "icons/${res}x${res}/apps/com.tencent.wechat.png" \
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
