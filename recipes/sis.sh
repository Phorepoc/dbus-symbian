#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# sis.sh -- the pdk's package.yml runs it after the components: D-Bus as $PHOREPOC_BUILD/sis/dbus-symbian-<arch>.sis (the DLLs, their resources, the licence texts and the
# source offer in \resource\dbus-symbian\licences). The package needs: GLib (installed first, by UID and version).
# Installs through the Catalogue; no image carries it. Resources are found by the programs on whatever drive it was installed to (glib-symbian overlay/phe_respath.h).
set -eu
ARCH=${PHOREPOC_ARCH:-x32}
REPO=$(cd "$(dirname "$0")/.." && pwd)
LIC=
for f in "$REPO"/LICENSES/* "$REPO/LICENSE" "$REPO/NOTICE" "$REPO/SOURCE-OFFER.txt"; do
  LIC="$LIC --file $f=!:/resource/dbus-symbian/licences/$(basename "$f")"
done
# --depends UID,0.1.0 = the lower layer at version >= 0.1.0 (its first release), no upper bound: any later release satisfies it
python3 "$PHOREPOC_PDK/tools/make_sis.py" phedbus phedbusglib phedbusd phebuscall --unsigned --version "${PHOREPOC_PACKAGE_VERSION:-1.0.0}" \
  --name "D-Bus" --vendor Phorepoc --depends 0xE7015772,0.1.0,"GLib" $LIC \
  --out "$PHOREPOC_BUILD/sis/dbus-symbian-$ARCH.sis"
