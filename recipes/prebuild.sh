#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# prebuild.sh -- the pdk's package.yml runs it in the toolchain container before the components: the GLib layer (glib-symbian's
# recipes/stack_env.sh), expat from gtk-symbian's fontconfig step (the daemon links it), then libdbus, dbus-glib and the bus daemon as
# the archives under $PHOREPOC_BUILD/hildon/ that registry/ links.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
glib_dir() {
  local r n i
  IFS=: read -r -a r <<< "${PHOREPOC_ADAPTATIONS:-}"; IFS=: read -r -a n <<< "${PHOREPOC_ADAPTATION_NAMES:-}"
  for i in "${!r[@]}"; do [ "${n[i]:-$(basename "${r[i]}")}" = glib-symbian ] && { echo "${r[i]}"; return 0; }; done
  echo "glib-symbian is not in PHOREPOC_ADAPTATIONS" >&2; return 1
}
. "$(glib_dir)/recipes/stack_env.sh"
. "$(adaptation_dir gtk-symbian)/recipes/stack_gtk.sh"
. "$HERE/recipes/stack_dbus.sh"
stack_glib
stack_gtk fetch freetype fontconfig
stack_dbus
