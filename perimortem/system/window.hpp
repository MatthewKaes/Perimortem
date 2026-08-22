// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#ifdef PERI_LINUX
#include "perimortem/system/platform/wayland/window.hpp"
#else
#error Perimortem does not have a window implementation for this platform.
#endif

namespace Perimortem::System {

#ifdef PERI_LINUX
// The public window owner remains System even while only one platform is
// implemented. Application composition may expose native presentation handles
// to a selected renderer, but Graphics does not acquire an OS dependency.
using Window = Platform::Wayland::Window;
#endif

}  // namespace Perimortem::System
