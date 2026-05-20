// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/context/names.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Compiler;

auto Context::Names::canonicalize(
    Perimortem::Core::View::Bytes module,
    Perimortem::Core::View::Bytes name) -> Perimortem::Core::View::Bytes {
  constexpr auto host_type = "TTX"_view;
  constexpr auto seperator = "_"_view;
  const auto size = host_type.get_size() + module.get_size() + name.get_size() +
                    (seperator.get_size() * 2);
  auto canon_name = arena.allocate(
      host_type.get_size() + module.get_size() + name.get_size() +
      (seperator.get_size() * 2));

  Writer::Textual writer(Access::Bytes(canon_name, size));
  writer << host_type << seperator << module << seperator << name;
  return View::Bytes(canon_name, size);
}

auto Context::Names::make_local_unique() -> View::Bytes {
  Count size = counter[3] ? 4 : counter[2] ? 3 : counter[1] ? 2 : 1;
  auto name = arena.allocate(size);

  switch (size) {
  case 4:
    name[size - 4] = '0' + counter[3];
  case 3:
    name[size - 3] = '0' + counter[2];
  case 2:
    name[size - 2] = '0' + counter[1];
  case 1:
    name[size - 1] = '0' + counter[0];
  }

  for (Int i = 0; i < 4; i++) {
    counter[i] += 1;
    if (counter[i] <= '9') {
      break;
    }

    counter[i] = 0;
  }

  return View::Bytes(name, size);
}
