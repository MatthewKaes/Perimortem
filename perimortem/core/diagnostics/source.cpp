// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/diagnostics/source.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Core::Diagnostics;

auto Source::is_set() const -> Bool {
  return impl != nullptr;
}

auto Source::get_line() const -> Count {
  if (impl == nullptr) {
    return 0;
  }
  return Count(impl->_M_line);
}

auto Source::get_column() const -> Count {
  if (impl == nullptr) {
    return 0;
  }
  return Count(impl->_M_column);
}

auto Source::get_file() const -> Core::View::Bytes {
  if (impl == nullptr) {
    return Core::View::Bytes();
  }
  return NullTerminated::to_view(impl->_M_file_name);
}

auto Source::get_function() const -> Core::View::Bytes {
  if (impl == nullptr) {
    return Core::View::Bytes();
  }
  return NullTerminated::to_view(impl->_M_function_name);
}
