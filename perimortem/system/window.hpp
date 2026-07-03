// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#ifdef PERI_LINUX
#include "perimortem/system/platform/wayland/window.hpp"
#else
#error Perimortem does not have a window implementation for this platform.
#endif

namespace Perimortem::System {

#ifdef PERI_LINUX
using Window = Platform::Wayland::Window;
#endif

}  // namespace Perimortem::System
