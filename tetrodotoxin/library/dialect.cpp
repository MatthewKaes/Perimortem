// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/dialect.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_16.hpp"
#include "tetrodotoxin/library/language/types/signed_32.hpp"
#include "tetrodotoxin/library/language/types/signed_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_32.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "tetrodotoxin/library/language/types/void.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static constexpr Library::Language::Types::Boolean boolean;
static constexpr Library::Language::Types::Unsigned_8 unsigned_8;
static constexpr Library::Language::Types::Unsigned_16 unsigned_16;
static constexpr Library::Language::Types::Unsigned_32 unsigned_32;
static constexpr Library::Language::Types::Unsigned_64 unsigned_64;
static constexpr Library::Language::Types::Signed_8 signed_8;
static constexpr Library::Language::Types::Signed_16 signed_16;
static constexpr Library::Language::Types::Signed_32 signed_32;
static constexpr Library::Language::Types::Signed_64 signed_64;
static constexpr Library::Language::Types::Real_32 real_32;
static constexpr Library::Language::Types::Real_64 real_64;
static constexpr Library::Language::Types::Void void_type;

static constexpr Static::Vector<Pair<View::Bytes, const Abstract*>, 12>
    intrinsic_source = {{
      Pair<View::Bytes, const Abstract*>{boolean.get_name(), &boolean},
      {unsigned_8.get_name(), &unsigned_8},
      {unsigned_16.get_name(), &unsigned_16},
      {unsigned_32.get_name(), &unsigned_32},
      {unsigned_64.get_name(), &unsigned_64},
      {signed_8.get_name(), &signed_8},
      {signed_16.get_name(), &signed_16},
      {signed_32.get_name(), &signed_32},
      {signed_64.get_name(), &signed_64},
      {real_32.get_name(), &real_32},
      {real_64.get_name(), &real_64},
      {void_type.get_name(), &void_type},
    }};

using Intrinsics = Table<const Abstract*, intrinsic_source>;

auto Library::Dialect::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Abstract& interpretation_context)
    -> Option<Tetrodotoxin::Language::Monograph&> {
  auto shared_materializations = materializations_for(domain, cursor);
  if (!shared_materializations) {
    return {};
  }

  auto& monograph = Library::Language::Monograph::create_authored(
      domain, documentation, *this, interpretation_context,
      *shared_materializations);

  // Each admitted declaration occupies its final Arena address and one exact
  // local name. Structure keeps its outer Cursor private until the closing
  // brace while the forward pass retains no discovery index or second graph.
  while (!cursor.matches(Code::Type::Terminal)) {
    const Documentation& declaration_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);

    // TTX keeps using in the ordinary Addressable space. Exact text dispatch
    // makes this Library grammar without adding another shared lexical Code.
    if (cursor.matches(Code::Type::Addressable) &&
        cursor.get_text() == "using"_view) {
      auto import = Library::Language::Import::parse(cursor);
      if (!import) {
        return {};
      }

      // Interpretation owns the open import inventory. A rejection here means
      // lifecycle work overlapped parsing, so the whole source transaction must
      // remain unpublished rather than silently dropping the authored route.
      Bool retained = monograph.retain_import(*import);
      if (!retained) {
        cursor.create_expression_error(
            import->get_span(),
            "Library Imports cannot enter a source after linking begins."_view);
        return {};
      }

      continue;
    }

    // This nonconsuming lookahead chooses the declaration family. The selected
    // owner parses the complete header and body inside one Cursor branch.
    if ((cursor.matches(Code::Type::Public) ||
         cursor.matches(Code::Type::Private)) &&
        cursor.peek(1).get_code() == Code::Type::Type) {
      Token declaration_kind = cursor.peek(3);
      if (declaration_kind.get_code() == Code::Type::Addressable &&
          declaration_kind.caculate_text(cursor.get_source_text()) ==
              "enum"_view) {
        auto enumeration = Library::Language::Types::Enumeration::interpret(
            domain, cursor, declaration_documentation, monograph);
        if (!enumeration) {
          return {};
        }

        if (!monograph.bind_static(
                *enumeration, enumeration->get_visibility())) {
          cursor.create_expression_error(
              enumeration->get_name_anchor(),
              "Duplicate Type name in this Library source."_view);
          return {};
        }

        continue;
      }

      if (declaration_kind.get_code() == Code::Type::Addressable) {
        View::Bytes kind =
            declaration_kind.caculate_text(cursor.get_source_text());
        if (kind == "struct"_view || kind == "object"_view) {
          auto structure = Library::Language::Types::Structure::interpret(
              domain, cursor, declaration_documentation, monograph,
              *shared_materializations);
          if (!structure) {
            return {};
          }

          if (!monograph.bind_static(*structure, structure->get_visibility())) {
            auto name_anchor = structure->get_name_anchor();
            if (name_anchor) {
              cursor.create_expression_error(
                  *name_anchor,
                  "Duplicate Static binding name in this Library source."_view);
            } else {
              cursor.create_token_error(
                  "Duplicate Static binding name in this Library source."_view);
            }
            return {};
          }

          continue;
        }
      }
    }

    auto function = Library::Language::Function::reserve(
        domain, cursor, declaration_documentation, monograph,
        monograph.get_source(), *shared_materializations);
    if (!function) {
      return {};
    }

    if (!function->complete(cursor)) {
      return {};
    }

    if (!monograph.bind_static(*function, function->get_visibility())) {
      cursor.create_token_error(
          "Duplicate Static binding name in this Library source."_view);
      return {};
    }
  }

  return monograph;
}

auto Library::Dialect::materializations_for(
    Allocator::Arena& domain,
    Cursor& cursor) -> Option<Language::Materializations&> {
  // One installed Dialect belongs to one Workspace Arena. Reusing its writer
  // keeps equal generated Types exact across every Monograph in that island.
  if (materializations) {
    if (&*materialization_domain != &domain) {
      cursor.create_token_error(
          "One installed Library Dialect cannot span two graph Arenas."_view);
      return {};
    }

    return *materializations;
  }

  auto& created = domain.construct<Language::Materializations>(domain);
  materialization_domain = domain;
  materializations = created;
  return created;
}

auto Library::Dialect::resolve_intrinsic(View::Bytes name) const
    -> const Abstract& {
  // Immutable Library Types keep one binary wide identity. The packed table
  // preserves their constexpr addresses without copying them into each
  // installed Dialect allowing the compiler to optimize a lot of the lookup.
  return *Intrinsics::find_or_default(name, &Invalid::get_invalid());
}

auto Library::Dialect::get_bool() -> const Ttx::Model::Types::Flag& {
  return boolean;
}

auto Library::Dialect::get_unsigned_8() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_8;
}

auto Library::Dialect::get_unsigned_16() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_16;
}

auto Library::Dialect::get_unsigned_32() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_32;
}

auto Library::Dialect::get_unsigned_64() -> const Ttx::Model::Types::Unsigned& {
  return unsigned_64;
}

auto Library::Dialect::get_signed_8() -> const Ttx::Model::Types::Signed& {
  return signed_8;
}

auto Library::Dialect::get_signed_16() -> const Ttx::Model::Types::Signed& {
  return signed_16;
}

auto Library::Dialect::get_signed_32() -> const Ttx::Model::Types::Signed& {
  return signed_32;
}

auto Library::Dialect::get_signed_64() -> const Ttx::Model::Types::Signed& {
  return signed_64;
}

auto Library::Dialect::get_real_32() -> const Ttx::Model::Types::Real& {
  return real_32;
}

auto Library::Dialect::get_real_64() -> const Ttx::Model::Types::Real& {
  return real_64;
}

auto Library::Dialect::get_void() -> const Ttx::Model::Type& {
  return void_type;
}
