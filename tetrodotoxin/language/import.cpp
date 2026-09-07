// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/import.hpp"

#include <string>

#include "ttx/query.hpp"

using namespace Tetrodotoxin::Language;
using namespace Perimortem;

Import::Import(Memory::Allocator::Arena& arena, const Description& description)
    : arena(arena), description(description), version_text([&] {
        const auto version = description.get_version();
        const std::string text = std::to_string(version.get_major()) + "." +
                                 std::to_string(version.get_minor());
        return arena.proxy(
            Core::View::Bytes(
                reinterpret_cast<const U8*>(text.data()), text.size()));
      }()) {}

auto Import::acquire(ttx_abstract authority) -> Bool {
  if (authority == nullptr || ttx_abstract_same(authority, get_abi()) ||
      ttx_abstract_same(authority, ttx_none())) {
    return False;
  }
  if (!ttx_abstract_same(acquired, ttx_unknown())) {
    return ttx_abstract_same(acquired, authority);
  }
  // Keep the source authority itself. Resolving it here would pin this import
  // to the graph current during acquisition and lose subsequent source edits.
  acquired = authority;
  target = authority;
  const auto route = get_route();
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); ++index) {
    const bool end = index == route.get_size();
    const bool separator = !end && index + 1 < route.get_size() &&
                           route[index] == ':' && route[index + 1] == ':';
    if (!end && !separator) {
      continue;
    }
    const auto name = route.slice(start, index - start);
    if (!name.is_empty()) {
      target = Reference::create(arena, target, name).get_abi();
    }
    if (separator) {
      ++index;
      start = index + 1;
    }
  }
  return True;
}

auto Import::resolve(ttx_abstract) const -> ttx_abstract {
  return Ttx::resolve(target);
}

auto Import::validate(Ttx::Lexical::Cursor& cursor) const -> Bool {
  const auto selected = resolve(get_abi());
  if (ttx_abstract_same(selected, ttx_unknown()) ||
      ttx_abstract_same(selected, ttx_none())) {
    auto report = cursor.create_report(get_expression_anchor());
    report << "Import `"_view << get_locator()
           << "` has no resolved source yet."_view;
    return False;
  }
  return True;
}

auto Import::dependency() -> tetrodotoxin_source_dependency {
  static const tetrodotoxin_source_dependency_ops operations = {
    .header =
        {sizeof(tetrodotoxin_source_dependency_ops), TTX_ABI_MAJOR,
         TTX_ABI_MINOR},
    .kind =
        [](tetrodotoxin_source_dependency_self* self) {
          return reinterpret_cast<Import*>(self)->get_kind() == Kind::Source
                     ? TETRODOTOXIN_DEPENDENCY_SOURCE
                     : TETRODOTOXIN_DEPENDENCY_PACKAGE;
        },
    .local_name =
        [](tetrodotoxin_source_dependency_self* self) -> ttx_borrowed_bytes {
      const auto name = reinterpret_cast<Import*>(self)->get_name();
      return {name.get_data(), name.get_size()};
    },
    .locator =
        [](tetrodotoxin_source_dependency_self* self) -> ttx_borrowed_bytes {
      const auto value = reinterpret_cast<Import*>(self)->get_locator();
      return {value.get_data(), value.get_size()};
    },
    .version =
        [](tetrodotoxin_source_dependency_self* self) -> ttx_borrowed_bytes {
      const auto value = reinterpret_cast<Import*>(self)->version_text;
      return {value.get_data(), value.get_size()};
    },
    .route =
        [](tetrodotoxin_source_dependency_self* self) -> ttx_borrowed_bytes {
      const auto value = reinterpret_cast<Import*>(self)->get_route();
      return {value.get_data(), value.get_size()};
    },
    .acquire =
        [](tetrodotoxin_source_dependency_self* self, ttx_abstract target,
           tetrodotoxin_dependency_result result) {
          const auto acquired =
              reinterpret_cast<Import*>(self)->acquire(target);
          if (acquired) {
            result.operations->acquired(result.self);
          } else {
            result.operations->rejected(result.self);
          }
        },
  };
  return {
    &operations, reinterpret_cast<tetrodotoxin_source_dependency_self*>(this)};
}
