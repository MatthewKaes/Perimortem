// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expressions/identifier.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/foreign/surface.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return alias.resolve();
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

auto Language::Expressions::Identifier::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Type&> access_scope) -> Bool {
  // The reserved Foreign spelling selects the one Source owned namespace
  // identity directly. Lexical context cannot substitute an ambient binding
  // or turn the Surface into a value Type.
  const Abstract& selected =
      name == "foreign"_view
          ? static_cast<const Abstract&>(
                static_cast<const Language::Monograph&>(source)
                    .get_source()
                    .get_foreign())
      : token.get_code() == Ttx::Lexical::Code::Type::Source
          ? resolve_alias(source.resolve_context(name))
      : token.get_code() == Ttx::Lexical::Code::Type::Type
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

  if (!selected.is<Type>() && !selected.is<Addressable>() &&
      !selected.is<Language::Foreign::Surface>()) {
    source.report(
        source_anchor,
        "Expression Identifier did not resolve to a Type, Addressable, or "
        "Foreign surface."_view,
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

auto Language::Expressions::Identifier::get_documentation() const
    -> const Documentation& {
  return result.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Abstract>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Expressions::Identifier::get_type() const -> const Abstract& {
  return result.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get().visit<Type>(
            [](const Type&) -> const Abstract& {
              return Dialect::get_descriptor();
            },
            [](const Abstract& addressable) -> const Abstract& {
              return addressable.visit<Addressable>(
                  [](const Addressable& selected) -> const Abstract& {
                    return selected.get_type();
                  },
                  [](const Abstract&) -> const Abstract& {
                    return Invalid::get_invalid();
                  });
            });
      });
}

auto Language::Expressions::Identifier::get_result() const -> const Abstract& {
  return result.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get();
      });
}
