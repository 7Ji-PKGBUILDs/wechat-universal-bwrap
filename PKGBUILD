# Maintainer: 7Ji <pugokushin at gmail dot com>
# Maintainer: leaeasy <leaeasy at gmail dot com>
# Maintainer: devome <evinedeng at hotmail dot com>

_pkgname=wechat-universal
pkgname=${_pkgname}-bwrap
pkgver=4.0.0.23
pkgrel=1
pkgdesc="WeChat (Universal) with bwrap sandbox"
arch=('x86_64' 'aarch64' 'loong64')
url="https://weixin.qq.com"
license=('proprietary' 'GPLv3') # GPLv3 as desktop-app was statically linked-in, refer: https://aur.archlinux.org/packages/wechat-universal-bwrap#comment-964013
install="${_pkgname}".install
provides=("${_pkgname}")
conflicts=("${_pkgname}")
replaces=('wechat-beta'{,-bwrap})
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
makedepends=(
    'patchelf'
)
if [[ "${CARCH}" == loong64 ]]; then # This is needed instead of a plain declaration because AUR web would return all depends regardless of client's arch, AUR helpers would thus refuse to build the package on non-loong64 due to missing liblol
    depends_loong64=('liblol')
fi
options=(!strip !debug)

source=(
    'fake_dde-file-manager'
    "${_pkgname}.sh"
    "${_pkgname}.desktop"
)

_deb_stem="com.tencent.wechat_${pkgver}"
_deb_url_common="https://home-store-packages.uniontech.com/appstore/pool/appstore/c/com.tencent.wechat/${_deb_stem}"

source_x86_64=("${_deb_url_common}_amd64.deb")
source_aarch64=("${_deb_url_common}_arm64.deb")
source_loong64=("${_deb_url_common}_loongarch64.deb")

noextract=("${_deb_stem}"_{amd,arm,loongarch}64.deb )

sha256sums=(
    'b25598b64964e4a38f8027b9e8b9a412c6c8d438a64f862d1b72550ac8c75164'
    'e3beb121edcb1e6f065226aec9137a7e38fd73af4030ee0e179427162a230fdc'
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
    declare -A _debian_arch=(
        ['x86_64']='amd64'
        ['aarch64']='arm64'
        ['loong64']='loongarch64'
    )
    echo 'Extracting data from deb...'
    bsdtar -xOf "${_deb_stem}_${_debian_arch[$CARCH]}.deb" ./data.tar.xz |
        xz -cdT0 |
        tar -x ./opt/apps/com.tencent.wechat

    mv opt/apps/com.tencent.wechat/{files,entries/icons} .
    rm files/libuosdevicea.so
    rm -rf opt

    local _file
    echo 'Patching rpath...'
    for _file in libwxtrans.so; do # add more here
        echo "  ${_file} => \$ORIGIN"
        patchelf --set-rpath '$ORIGIN' files/"${_file}"
    done

    echo 'Stripping executable permission of non-ELF files...'
    local _file
    for _file in $(find files -type f -perm /111); do
        readelf -h "${_file}" &>/dev/null && continue || true
        stat --printf '  %A => ' "${_file}"
        chmod u-x,g-x,o-x "${_file}"
        stat --format '%A %n' "${_file}"
    done
}

package() {
    echo 'Popupating pkgdir with earlier extracted data...'
    mkdir -p "${pkgdir}"/opt
    cp -r --preserve=mode files "${pkgdir}/opt/${_pkgname}"

    echo 'Installing icons...'
    for res in 16 32 48 64 128 256; do
        install -Dm644 \
            "icons/hicolor/${res}x${res}/apps/com.tencent.wechat.png" \
            "${pkgdir}/usr/share/icons/hicolor/${res}x${res}/apps/${_pkgname}.png"
    done
    rm -rf "${pkgdir}"/opt/apps

    local _wechat_root="${pkgdir}/usr/lib/${_pkgname}"

    echo 'Installing scripts...'
    install -Dm755 wechat-universal.sh "${_wechat_root}"/common.sh
    ln -s common.sh "${_wechat_root}"/start.sh
    ln -s common.sh "${_wechat_root}"/stop.sh
    mkdir -p "${pkgdir}"/usr/bin
    ln -s ../lib/"${_pkgname}"/common.sh "${pkgdir}"/usr/bin/"${_pkgname}"

    echo 'Installing fake deepin file manager...'
    install -Dm755 {fake_,"${_wechat_root}"/usr/bin/}dde-file-manager

    echo 'Installing desktop files...'
    install -Dm644 "${_pkgname}.desktop" "${pkgdir}/usr/share/applications/${_pkgname}.desktop"
}
