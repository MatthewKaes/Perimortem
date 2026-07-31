// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {

// Owns the installed Dialect inventory for one Workspace. Every instance uses
// the shared graph Arena and registry, then remains alive until Retention has
// destroyed each Monograph hosted by that Dialect.
class Dialects {
 public:
  Dialects(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& registry);
  ~Dialects();

  template <typename TargetDialect>
  auto install(Perimortem::Core::View::Bytes name) -> Bool {
    if (installed.contains(name)) {
      return false;
    }

    Perimortem::Core::View::Bytes retained_name = arena.proxy(name);
    auto& dialect = arena.construct<TargetDialect>(registry);
    Installed retained(dialect);

    names.insert(retained_name);
    values.insert(retained);

    // Map values are nonassignable references. The exact duplicate guard above
    // makes launder a construction operation rather than replacement policy.
    installed.launder(retained_name, dialect);
    return true;
  }

  auto find(Perimortem::Core::View::Bytes name)
      -> Perimortem::Utility::Option<Language::Dialect&>;
  auto get_names() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes>;

 private:
  class Installed {
   public:
    explicit Installed(Language::Dialect& value);
    auto get() const -> Language::Dialect&;

   private:
    Language::Dialect& value;
  };

  Perimortem::Memory::Allocator::Arena& arena;
  Ttx::Concept::Abstract& registry;
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Vector<Installed> values;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Language::Dialect&>
          installed;
};

}  // namespace Tetrodotoxin::Environment
