// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/identifier.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static const Layouts::Fluid identifier_inputs;

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return alias.resolve();
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

auto Language::Identifier::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Materializations&,
    Core::Option<const Type&> access_scope) -> Bool {
  const Abstract& selected =
      token.get_code() == Ttx::Lexical::Code::Type::Type
          ? access_scope.visit(
                [&]() -> const Abstract& {
                  return resolve_alias(lexical_context.resolve_context(name));
                },
                [&](const Type& caller) -> const Abstract& {
                  return caller.visit<Language::Types::Composite>(
                      [&](const Language::Types::Composite& composite)
                          -> const Abstract& {
                        return resolve_alias(
                            composite.resolve_type_root(name, caller));
                      },
                      [&](const Abstract&) -> const Abstract& {
                        return resolve_alias(
                            lexical_context.resolve_context(name));
                      });
                })
          : resolve_alias(lexical_context.resolve_context(name));
  auto source_anchor = get_anchor();

  if (!selected.is<Type>() && !selected.is<Addressable>()) {
    source.report(
        source_anchor,
        "Expression Identifier did not resolve to a Type or Addressable."_view,
        "Publish the named semantic object before linking this use."_view);
    return False;
  }

  // A later pass may fill an unresolved name, but a successful edge is
  // permanent. Repeating the same exact link remains harmless.
  if (result && &result->get() != &selected) {
    source.report(
        source_anchor,
        "Expression Identifier cannot change its linked result."_view,
        "Keep one exact semantic object bound to this authored Token."_view);
    return False;
  }

  result = Reference<const Abstract>(selected);
  return True;
}

auto Language::Identifier::get_documentation() const -> const Documentation& {
  return result.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Abstract>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Identifier::get_type() const -> const Abstract& {
  return result.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get().visit<Type>(
            [](const Type&) -> const Abstract& {
              return Dialect::get_descriptor();
            },
            [](const Abstract& addressable) -> const Abstract& {
              return static_cast<const Addressable&>(addressable).get_type();
            });
      });
}

auto Language::Identifier::get_result() const -> const Abstract& {
  return result.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Identifier::get_inputs() const -> const Layout& {
  return identifier_inputs;
}
