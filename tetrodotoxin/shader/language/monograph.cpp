// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/monograph.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Shader::Language::Monograph::create(
    Allocator::Arena& domain,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context,
    Library::Language::Monograph& library) -> Monograph& {
  return domain.construct_from<Monograph>([&]() {
    return Monograph(domain, language, documentation, context, library);
  });
}

auto Shader::Language::Monograph::retain_program(Program& program) -> Bool {
  for (const Reference<Program>& retained : programs.get_view()) {
    BAIL_IF(retained.get().get_name() == program.get_name());
  }
  programs.insert(program);
  return True;
}

auto Shader::Language::Monograph::retain_bridge(Bridge& bridge) -> Bool {
  for (const Reference<Bridge>& retained : bridges.get_view()) {
    BAIL_IF(retained.get().get_name() == bridge.get_name());
  }
  bridges.insert(bridge);
  return True;
}

auto Shader::Language::Monograph::compose(Cursor& cursor) -> Bool {
  Bool valid = True;
  for (Reference<Program> program : programs.get_view()) {
    valid &= program.get().compose_contract(cursor);
  }
  return valid;
}

auto Shader::Language::Monograph::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }
  Bool valid = library.link(cursor);
  for (Reference<Bridge> bridge : bridges.get_view()) {
    valid &= bridge.get().link(cursor, *this);
  }
  linked = valid;
  return valid;
}

auto Shader::Language::Monograph::finalize(Cursor& cursor) -> Bool {
  if (finalized) {
    return True;
  }
  Bool valid = library.finalize(cursor);
  for (Reference<Program> program : programs.get_view()) {
    valid &= program.get().validate_contract(cursor);
  }
  finalized = valid;
  return valid;
}

auto Shader::Language::Monograph::compose_restored() -> Bool {
  Bool valid = True;
  for (Reference<Program> program : programs.get_view()) {
    valid &= program.get().compose_contract_restored();
  }
  return valid;
}

auto Shader::Language::Monograph::link_restored() -> Bool {
  if (linked) {
    return True;
  }

  Bool valid = library.link_restored();
  for (Reference<Bridge> bridge : bridges.get_view()) {
    valid &= bridge.get().link_restored(*this);
  }
  linked = valid;
  return valid;
}

auto Shader::Language::Monograph::finalize_restored() -> Bool {
  if (finalized) {
    return True;
  }

  Bool valid = library.finalize_restored();
  if (!valid) {
    Diagnostics::Log::error(
        "Shader Archive could not finalize its restored Library child."_view);
  }
  for (Reference<Program> program : programs.get_view()) {
    if (!program.get().validate_contract_restored()) {
      Diagnostics::Log::Message<256> message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      message << "Shader Archive Program `"_view << program.get().get_name()
              << "` no longer satisfies its restored Render contract."_view;
      valid = False;
    }
  }
  finalized = valid;
  return valid;
}

auto Shader::Language::Monograph::get_layer(const Abstract& requested) const
    -> Option<const Tetrodotoxin::Language::Monograph&> {
  auto outer = Tetrodotoxin::Language::Monograph::get_layer(requested);
  if (outer) {
    return *outer;
  }
  return &requested == &library.get_language()
             ? Option<const Tetrodotoxin::Language::Monograph&>(library)
             : Option<const Tetrodotoxin::Language::Monograph&>();
}

auto Shader::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  for (const Reference<Program>& program : programs.get_view()) {
    if (program.get().get_name() == route) {
      return program.get();
    }
  }
  for (const Reference<Bridge>& bridge : bridges.get_view()) {
    if (bridge.get().get_name() == route) {
      return bridge.get();
    }
  }

  const Abstract& child = library.resolve_context(route);
  return child.is<Invalid>()
             ? Tetrodotoxin::Language::Monograph::resolve_context(route)
             : child;
}
