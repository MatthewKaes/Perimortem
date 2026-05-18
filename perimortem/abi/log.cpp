// Perimortem Engine
// Copyright © Matt Kaes

#include <stdio.h>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;

extern "C" void print(View::Bytes data) {
  fflush(stdout);
  View::Bytes debug_text = "[TTX DEBUG] "_view;
  View::Bytes end_line = "\n"_view;
  fwrite(debug_text.get_data(), 1, debug_text.get_size(), stdout);
  fwrite(data.get_data(), 1, data.get_size(), stdout);
  fwrite(end_line.get_data(), 1, end_line.get_size(), stdout);
}
