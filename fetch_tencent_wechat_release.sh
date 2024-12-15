#!/bin/bash
# Fetcher for tencent wechat release
# Copyright (C) 2024-present Guoxin "7Ji" Pu

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.

# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


# A full package URL looks like this:
#   https://dldir1v6.qq.com/weixin/Universal/Linux/WeChatLinux_x86_64.deb

set -euo pipefail

declare -A arches_tencent_from_archlinux=(
    ['x86_64']='x86_64'
    ['aarch64']='arm64'
    ['loong64']='LoongArch'
)

name_prefix='wechat-universal-'
url_prefix='https://dldir1v6.qq.com/weixin/Universal/Linux/WeChatLinux_'

version_max=0
body=
declare -A names urls sha256sums

for arch_archlinux in x86_64 aarch64 loong64; do
    url="${url_prefix}${arches_tencent_from_archlinux["${arch_archlinux}"]}.deb"
    name_temp="${name_prefix}unknown-${arch_archlinux}.deb.temp"
    curl -vLo "${name_temp}" "${url}"
    version=$(
        bsdtar -xOf "${name_temp}" ./control.tar.xz |
            xz -cdT0 |
            tar -xO ./control |
            sed -n 's/^Version: \+\(.\+\)$/\1/p'
    )
    if vercmp "${version}" "${version_max}"; then
        version_max="${version}"
    fi
    sha256sum_deb=$(sha256sum "${name_temp}")
    name="${name_prefix}${version}-${arch_archlinux}.deb"
    mv "${name_temp}" "${name}"
    names["${arch_archlinux}"]="${name}"
    urls["${arch_archlinux}"]="${url}"
    sha256sums["${arch_archlinux}"]="${sha256sum_deb::64}"
done

echo "pkgver=${version_max}"
for arch_archlinux in x86_64 aarch64 loong64; do
    printf "sources_%s=(\n    '%s::%s'\n)\n" \
        "${arch_archlinux}" "${names["${arch_archlinux}"]}" \
        "${urls["${arch_archlinux}"]}"
done
for arch_archlinux in x86_64 aarch64 loong64; do
    printf "sha256sums_%s=(\n    '%s'\n)\n" \
        "${arch_archlinux}" "${sha256sums["${arch_archlinux}"]}"
done