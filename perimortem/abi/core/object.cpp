// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/abi/core/object.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem;

static auto select_object(Unsigned_8* payload) -> Core::Object {
  if (!payload) {
    Core::Diagnostics::Log::fatal(
        "Core Object ABI received an empty handle."_view);
  }

  return Core::Object(payload);
}

extern "C" auto perimortem_core_object_allocate(
    const Core::Object::Descriptor* descriptor) -> Unsigned_8* {
  if (!descriptor) {
    Core::Diagnostics::Log::fatal(
        "Core Object ABI received an empty descriptor."_view);
  }

  return Core::Object::create(*descriptor).get_payload();
}

extern "C" auto perimortem_core_object_retain(Unsigned_8* payload) -> void {
  select_object(payload).retain();
}

extern "C" auto perimortem_core_object_release(Unsigned_8* payload) -> void {
  select_object(payload).release();
}
