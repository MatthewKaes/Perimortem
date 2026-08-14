// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "ttx/concept/type_identity.hpp"

namespace Tetrodotoxin::Environment {

// Owns the installed Dialect inventory for one Workspace. Every instance uses
// the shared graph Arena, then remains alive until Retention has destroyed each
// Monograph hosted by that Dialect.
class Dialects {
 public:
  explicit Dialects(Perimortem::Memory::Allocator::Arena& arena);
  ~Dialects();

  template <typename TargetDialect, typename... DependencyDialects>
  auto install(
      Perimortem::Core::View::Bytes name,
      DependencyDialects&... dependencies) -> TargetDialect* {
    Unsigned_64 type_identity =
        Ttx::Concept::get_type_identity<TargetDialect>();
    if (installed.contains(name) || contains_type(type_identity)) {
      return nullptr;
    }

    Bool dependencies_installed =
        (contains_instance(static_cast<Language::Dialect&>(dependencies)) &&
         ...);
    if (!dependencies_installed) {
      return nullptr;
    }

    Perimortem::Core::View::Bytes retained_name = arena.proxy(name);
    auto& dialect = arena.construct<TargetDialect>(dependencies...);
    Installed retained(dialect, type_identity);

    names.insert(retained_name);
    values.insert(retained);

    // Map values are nonassignable references. The exact duplicate guard above
    // makes launder a construction operation rather than replacement policy.
    installed.launder(retained_name, dialect);
    return &dialect;
  }

  auto find(Perimortem::Core::View::Bytes name)
      -> Perimortem::Core::Option<Language::Dialect&>;
  auto get_names() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>;

 private:
  class Installed {
   public:
    Installed(Language::Dialect& value, Unsigned_64 type_identity);
    auto get() const -> Language::Dialect&;
    auto get_type_identity() const -> Unsigned_64;

   private:
    Language::Dialect& value;
    Unsigned_64 type_identity;
  };

  auto contains_instance(const Language::Dialect& dialect) const -> Bool;
  auto contains_type(Unsigned_64 type_identity) const -> Bool;

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Vector<Installed> values;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Dialect&>
          installed;
};

}  // namespace Tetrodotoxin::Environment
