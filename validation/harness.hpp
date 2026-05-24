// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Validation {

inline auto do_nothing() -> void {};

using InitFunc = void (*)();
using SetupFunc = void (*)();
using TeardownFunc = void (*)();

struct Harness {
  Perimortem::Core::View::Bytes name = ""_view;
  InitFunc init = do_nothing;
  SetupFunc setup = do_nothing;
  TeardownFunc teardown = do_nothing;
  Count batch_count = 1;
};

}  // namespace Validation
