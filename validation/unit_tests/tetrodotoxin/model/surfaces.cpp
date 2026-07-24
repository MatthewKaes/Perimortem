// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/model/addressables/field.hpp"
#include "tetrodotoxin/model/callables/self.hpp"
#include "tetrodotoxin/model/callables/static.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/types/members.hpp"
#include "tetrodotoxin/model/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressables/writable.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Ttx::Model;
using namespace Ttx::Concept;
using namespace Validation;

static Harness ModelSurfaces = {
  .name = "Tetrodotoxin::Ttx::Model::Surfaces"_view,
};

class NamedType final : public Ttx::Model::Type {
 public:
  constexpr explicit NamedType(View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  constexpr auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

PERIMORTEM_UNIT_TEST(
    ModelSurfaces,
    namespace_separates_retained_and_exported_definitions) {
  Allocator::Arena arena;
  Namespace definitions(arena, "Library"_view);
  Namespace hidden(arena, "Hidden"_view);
  Namespace published(arena, "Published"_view);
  Namespace duplicate(arena, "Published"_view);
  NamedType count("Count"_view);
  Addressables::Field late_state("late_state"_view, count);
  View::Vector<Reference<Ttx::Model::Addressable>> parameters;
  View::Vector<Reference<Abstract>> results;
  Callables::Static late_static(
      arena, "late_static"_view, parameters, results,
      Documentation::get_empty());

  Bool retained = definitions.add_root(hidden);
  Bool exported = definitions.add_export(published);

  EXPECT(retained);
  EXPECT(exported);
  EXPECT_EQ(definitions.get_root_count(), Count(2));
  EXPECT_EQ(definitions.get_export_count(), Count(1));
  EXPECT(&definitions.get_root(0) == &hidden);
  EXPECT(&definitions.get_root(1) == &published);
  EXPECT(&definitions.get_export(0) == &published);
  EXPECT(&definitions.resolve_root("Hidden"_view) == &hidden);
  EXPECT(definitions.resolve_context("Hidden"_view).is<Invalid>());
  EXPECT(&definitions.resolve_context("Published"_view) == &published);

  Bool duplicated = definitions.add_export(duplicate);

  EXPECT_NOT(duplicated);
  EXPECT_EQ(definitions.get_root_count(), Count(2));
  EXPECT_EQ(definitions.get_export_count(), Count(1));

  Bool sealed = definitions.seal();
  Bool sealed_twice = definitions.seal();
  Namespace late(arena, "Late"_view);
  Bool root_after_seal = definitions.add_root(late);
  Bool export_after_seal = definitions.add_export(late);
  Bool exposure_after_seal = definitions.add_exposed(late_state);
  Bool static_after_seal = definitions.add_static(late_static);
  Bool exported_static_after_seal =
      definitions.add_exported_static(late_static);

  EXPECT(sealed);
  EXPECT_NOT(sealed_twice);
  EXPECT_NOT(root_after_seal);
  EXPECT_NOT(export_after_seal);
  EXPECT_NOT(exposure_after_seal);
  EXPECT_NOT(static_after_seal);
  EXPECT_NOT(exported_static_after_seal);
  EXPECT_EQ(definitions.get_root_count(), Count(2));
}

PERIMORTEM_UNIT_TEST(
    ModelSurfaces,
    namespace_rejects_names_already_visible_in_an_outer_context) {
  Allocator::Arena arena;
  Namespace outer(arena, "Outer"_view);
  Namespace existing(arena, "Taken"_view);
  Namespace inner(arena, "Inner"_view);
  Namespace shadow(arena, "Taken"_view);

  Bool outer_added = outer.add_export(existing);
  Bool outer_sealed = outer.seal();
  Bool inner_added = inner.add_root(shadow, outer);

  EXPECT(outer_added);
  EXPECT(outer_sealed);
  EXPECT_NOT(inner_added);
  EXPECT_EQ(inner.get_root_count(), Count(0));
  EXPECT(inner.resolve_root("Taken"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(
    ModelSurfaces,
    namespace_exposes_read_only_state_and_indexes_static_calls) {
  Allocator::Arena arena;
  NamedType count("Count"_view);
  Addressables::Field state("counter"_view, count);
  View::Vector<Reference<Ttx::Model::Addressable>> parameters;
  View::Vector<Reference<Abstract>> results;
  Callables::Static hidden(
      arena, "hidden"_view, parameters, results, Documentation::get_empty());
  Callables::Static entry(
      arena, "entry"_view, parameters, results, Documentation::get_empty());
  Callables::Static duplicate(
      arena, "entry"_view, parameters, results, Documentation::get_empty());
  Callables::Self receiver(
      arena, "receiver"_view, parameters, results, Documentation::get_empty());
  Namespace definitions(arena, "Library"_view);

  Bool exposed = definitions.add_exposed(state);
  Bool hidden_added = definitions.add_static(hidden);
  Bool entry_added = definitions.add_exported_static(entry);
  Bool duplicate_added = definitions.add_exported_static(duplicate);
  Bool receiver_added = definitions.add_root(receiver);

  const Abstract& public_state = definitions.resolve_context("counter"_view);
  EXPECT(exposed);
  EXPECT(hidden_added);
  EXPECT(entry_added);
  EXPECT_NOT(duplicate_added);
  EXPECT_NOT(receiver_added);
  EXPECT_EQ(definitions.get_root_count(), Count(3));
  EXPECT_EQ(definitions.get_export_count(), Count(2));
  EXPECT(&definitions.resolve_root("counter"_view) == &state);
  EXPECT(&definitions.resolve_root("hidden"_view) == &hidden);
  EXPECT(&definitions.resolve_root("entry"_view) == &entry);
  EXPECT(&definitions.get_export(0) == &public_state);
  EXPECT(&definitions.get_export(1) == &entry);
  EXPECT(&public_state != &state);
  EXPECT(public_state.is<Ttx::Model::Addressable>());
  EXPECT_NOT(public_state.is<Ttx::Model::Addressables::Writable>());
  EXPECT(
      &public_state.assume<Ttx::Model::Addressable>().get_type().resolve() ==
      &state.get_type().resolve());
  EXPECT(&definitions.resolve_static("hidden"_view) == &hidden);
  EXPECT(&definitions.resolve_static("entry"_view) == &entry);
  EXPECT(definitions.resolve_exported_static("hidden"_view).is<Invalid>());
  EXPECT(&definitions.resolve_exported_static("entry"_view) == &entry);
}

PERIMORTEM_UNIT_TEST(
    ModelSurfaces,
    members_keep_named_static_and_self_surfaces_independent) {
  Allocator::Arena arena;
  NamedType count("Count"_view);
  Addressables::Field value("value"_view, count);
  View::Vector<Reference<Ttx::Model::Addressable>> parameters;
  View::Vector<Reference<Abstract>> results;
  Callables::Static static_identity(
      arena, "identity"_view, parameters, results, Documentation::get_empty());
  Callables::Self self_identity(
      arena, "identity"_view, parameters, results, Documentation::get_empty());
  Callables::Static duplicate_static(
      arena, "identity"_view, parameters, results, Documentation::get_empty());
  Callables::Self duplicate_self(
      arena, "identity"_view, parameters, results, Documentation::get_empty());
  Namespace outer(arena, "Outer"_view);
  Namespace outer_name(arena, "Taken"_view);
  Addressables::Field shadow("Taken"_view, count);
  Types::Members members(arena);

  Bool outer_added = outer.add_export(outer_name);
  Bool outer_sealed = outer.seal();
  Bool shadow_added = members.add_root(shadow, outer);
  Bool value_added = members.add_export(value);
  Bool static_added = members.add_exported_static(static_identity);
  Bool self_added = members.add_exported_self(self_identity);
  Bool static_duplicated = members.add_static(duplicate_static);
  Bool self_duplicated = members.add_self(duplicate_self);

  EXPECT(outer_added);
  EXPECT(outer_sealed);
  EXPECT_NOT(shadow_added);
  EXPECT(value_added);
  EXPECT(static_added);
  EXPECT(self_added);
  EXPECT_NOT(static_duplicated);
  EXPECT_NOT(self_duplicated);
  EXPECT_EQ(members.get_root_count(), Count(3));
  EXPECT_EQ(members.get_export_count(), Count(3));
  EXPECT(&members.get_root(0) == &value);
  EXPECT(&members.get_root(1) == &static_identity);
  EXPECT(&members.get_root(2) == &self_identity);
  EXPECT(&members.resolve_context("value"_view) == &value);
  EXPECT(members.resolve_context("identity"_view).is<Invalid>());
  EXPECT(&members.resolve_static("identity"_view) == &static_identity);
  EXPECT(&members.resolve_self("identity"_view) == &self_identity);
  EXPECT(&members.resolve_exported_static("identity"_view) == &static_identity);
  EXPECT(&members.resolve_exported_self("identity"_view) == &self_identity);

  Bool sealed = members.seal();
  Addressables::Field late("late"_view, count);
  Callables::Static late_static(
      arena, "late_static"_view, parameters, results,
      Documentation::get_empty());
  Callables::Self late_self(
      arena, "late_self"_view, parameters, results, Documentation::get_empty());
  Bool root_after_seal = members.add_root(late);
  Bool export_after_seal = members.add_export(late);
  Bool exposure_after_seal = members.add_exposed(late);
  Bool static_after_seal = members.add_static(late_static);
  Bool exported_static_after_seal = members.add_exported_static(late_static);
  Bool self_after_seal = members.add_self(late_self);
  Bool exported_self_after_seal = members.add_exported_self(late_self);

  EXPECT(sealed);
  EXPECT_NOT(root_after_seal);
  EXPECT_NOT(export_after_seal);
  EXPECT_NOT(exposure_after_seal);
  EXPECT_NOT(static_after_seal);
  EXPECT_NOT(exported_static_after_seal);
  EXPECT_NOT(self_after_seal);
  EXPECT_NOT(exported_self_after_seal);
  EXPECT_EQ(members.get_root_count(), Count(3));
  EXPECT_EQ(members.get_export_count(), Count(3));
}

PERIMORTEM_UNIT_TEST(
    ModelSurfaces,
    structure_completes_and_seals_every_member_surface) {
  Allocator::Arena arena;
  NamedType count("Count"_view);
  Addressables::Field hidden("hidden"_view, count);
  Addressables::Field published("published"_view, count);
  Addressables::Field exposed("exposed"_view, count);
  Addressables::Field late("late"_view, count);
  View::Vector<Reference<Ttx::Model::Addressable>> parameters;
  View::Vector<Reference<Abstract>> results;
  Callables::Static static_identity(
      arena, "identity"_view, parameters, results, Documentation::get_empty());
  Callables::Self self_identity(
      arena, "identity"_view, parameters, results, Documentation::get_empty());
  Types::Structure structure(
      arena, "Structure"_view, Documentation::get_empty());

  Bool hidden_added = structure.add_field(hidden);
  Bool published_added = structure.add_exported_field(published);
  Bool exposed_added = structure.add_exposed_field(exposed);
  Bool static_added = structure.add_exported_static(static_identity);
  Bool self_added = structure.add_exported_self(self_identity);

  EXPECT(hidden_added);
  EXPECT(published_added);
  EXPECT(exposed_added);
  EXPECT(static_added);
  EXPECT(self_added);
  EXPECT(structure.resolve().is<Invalid>());
  EXPECT(&structure.resolve_root("hidden"_view) == &hidden);
  EXPECT(&structure.resolve_static("identity"_view) == &static_identity);
  EXPECT(structure.resolve_exported_static("identity"_view).is<Invalid>());

  Bool completed = structure.complete();
  const Abstract& public_exposed = structure.resolve_context("exposed"_view);

  EXPECT(completed);
  EXPECT(&structure.resolve() == &structure);
  EXPECT_EQ(structure.get_layout().get_size(), Count(3));
  EXPECT_EQ(structure.get_member_count(), Count(5));
  EXPECT_EQ(structure.get_export_count(), Count(4));
  EXPECT(structure.resolve_context("hidden"_view).is<Invalid>());
  EXPECT(&structure.resolve_context("published"_view) == &published);
  EXPECT(&structure.resolve_root("exposed"_view) == &exposed);
  EXPECT(&public_exposed != &exposed);
  EXPECT_NOT(public_exposed.is<Ttx::Model::Addressables::Writable>());
  EXPECT(
      &structure.resolve_exported_static("identity"_view) == &static_identity);
  EXPECT(&structure.resolve_exported_self("identity"_view) == &self_identity);

  Bool field_after_completion = structure.add_field(late);
  Bool member_after_completion = structure.add_exported_member(late);
  Bool completed_twice = structure.complete();
  Bool representation_after_completion = structure.set_shader_type(count);

  EXPECT_NOT(field_after_completion);
  EXPECT_NOT(member_after_completion);
  EXPECT_NOT(completed_twice);
  EXPECT_NOT(representation_after_completion);
  EXPECT_EQ(structure.get_member_count(), Count(5));
}
