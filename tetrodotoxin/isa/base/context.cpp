// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/context.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;

auto Base::Context::read_embedded(
    View::Bytes source,
    View::Bytes relative,
    View::Bytes& content) -> Bool {
  return embedded_reader != nullptr &&
         embedded_reader(arena, source, relative, content);
}

auto Base::Context::parameterize_type(
    const Ttx::Type& root,
    Ttx::Layout arguments,
    Count extent,
    View::Bytes name) -> const Ttx::Type& {
  for (Count i = 0; i < parameterizations.get_size(); i++) {
    if (parameterizations[i].matches(root, arguments, extent)) {
      return parameterizations[i].get_result();
    }
  }

  Managed::Bytes stored_name(arena, name);
  const Ttx::Type& result = arena.construct<Ttx::Type>(stored_name.get_view());

  Managed::Vector<Ttx::Member> stored_arguments(arena);
  View::Vector<Ttx::Member> members = arguments.get_members();
  for (Count i = 0; i < members.get_size(); i++) {
    stored_arguments.insert(members[i]);
  }

  parameterizations.insert(Parameterization(
      root, Ttx::Layout(stored_arguments.get_view()), extent, result));
  return result;
}
