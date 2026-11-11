# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause

"""components_dbus -- dbus-symbian's components (merged into core's registry when this repository is a layer:
PHOREPOC_ADAPTATIONS=...:<glib-symbian>:<dbus-symbian>). ".hildon/..." resolves to $PHOREPOC_BUILD/hildon/ (the archives
recipes/ builds). Nothing here is in an OS image: D-Bus is a Catalogue download (recipes/sis.sh).
"""
import os

from paths import OS_ROOT

_COMMON = dict(allow_wsd=True, static_ctors=True, rtti=False, defs=["__SOFTFP="])
_RT = [f"thirdparty/x86/cxxrt/{a}.a" for a in ("libphcompat", "libgcc")]
_DATA = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "os", "data")


def _dll(name):
    return dict(mmp=os.path.join(OS_ROOT, f"src/{name}/{name}.mmp"), rom=f"{name}.dll", cdriver=True,
                x32_archives=[f".hildon/lib{name}.a"] + _RT, **{"def": os.path.join(OS_ROOT, f"src/{name}/{name}.def")}, **_COMMON)


COMPONENTS = {
    "phedbus": _dll("phedbus"),
    "phedbusglib": _dll("phedbusglib"),
    # the session bus daemon: one executable (libdbus internals + expat); its config is data
    "phedbusd": dict(mmp=os.path.join(OS_ROOT, "src/phedbusd/phedbusd.mmp"), stack=0x100000,
                     rom_data=["resource\\dbus\\session.conf=" + os.path.join(_DATA, "dbus/session.conf"),
                               "resource\\dbus\\services\\com.nokia.HildonHome.service="
                               + os.path.join(_DATA, "dbus/services/com.nokia.HildonHome.service")],
                     x32_archives=[".hildon/libphedbusd.a", ".hildon/libphedbus.a", ".hildon/libpheexpat.a"] + _RT, **_COMMON),
    # dbus-send like: phebuscall DEST PATH INTERFACE METHOD [TYPE:VALUE ...]
    "phebuscall": dict(mmp=os.path.join(OS_ROOT, "src/phebuscall/phebuscall.mmp"), stack=0x100000,
                       x32_archives=[".hildon/libphebuscall.a"] + _RT, **_COMMON),
}

# installed, not in a ROM
for _c in COMPONENTS.values():
    _c.setdefault("sis", True)
