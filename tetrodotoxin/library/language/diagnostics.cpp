// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/diagnostics.hpp"

#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Diagnostics::write_type(
    Ttx::Lexical::Errors::Report& report,
    const Abstract& abstract) -> void {
  const Abstract& resolved = abstract.is<Language::Model::Type>() ||
                                     abstract.is<Language::Model::Addressable>()
                                 ? abstract
                                 : abstract.resolve();
  auto type = resolved.select<Language::Model::Type>();
  if (!type) {
    type = resolved.visit<Language::Model::Addressable>(
        [](const Language::Model::Addressable& addressable)
            -> Option<const Language::Model::Type&> {
          return addressable.get_type();
        },
        [](const Abstract&) -> Option<const Language::Model::Type&> {
          return {};
        });
  }

  if (!type) {
    auto pack = resolved.select<Language::Model::Pack>();
    if (pack && pack->get_layout().get_size() == 1) {
      const Abstract& value_type = pack->get_value_type(0);
      if (&value_type != &resolved) {
        write_type(report, value_type);
        return;
      }
    }
  }

  if (!type || type->is<Invalid>()) {
    report << "<invalid>"_view;
    return;
  }

  report << type->get_name();
}

auto Language::Diagnostics::write_layout(
    Ttx::Lexical::Errors::Report& report,
    const Layout& layout) -> void {
  report << "["_view;
  for (Count index = 0; index < layout.get_size(); index++) {
    if (index != 0) {
      report << ", "_view;
    }

    auto name = layout.get_name(index);
    if (name) {
      report << "."_view << *name << " = "_view;
    }

    layout.get_abstract(index).visit(
        [&]() { report << "<invalid>"_view; },
        [&](const Abstract& selected) { write_type(report, selected); });
  }
  report << "]"_view;
}

auto Language::Diagnostics::write_pack(
    Ttx::Lexical::Errors::Report& report,
    const Model::Pack& pack) -> void {
  const Layout& layout = pack.get_layout();
  report << "["_view;
  for (Count index = 0; index < layout.get_size(); index++) {
    if (index != 0) {
      report << ", "_view;
    }

    auto name = layout.get_name(index);
    if (name) {
      report << "."_view << *name << " = "_view;
    }
    write_type(report, pack.get_value_type(index));
  }
  report << "]"_view;
}
