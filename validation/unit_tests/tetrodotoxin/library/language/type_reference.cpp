// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/type_reference.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness LibraryTypeReference = {
  .name = "Tetrodotoxin::Library::Language::TypeReference"_view,
};

static auto interpret(
    Workspace& workspace,
    Errors& errors,
    View::Bytes semantic_name,
    View::Bytes source) -> Option<Language::Monograph&> {
  BAIL_IF(!workspace.install_dialect<Dialect>("Library"_view));

  auto interpreted = workspace.interpret_source(
      errors, semantic_name, "type-reference.ttx"_view, source);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  for (const Reference<Abstract>& binding : composite.get_addressables()) {
    if (binding.get().get_name() == name &&
        binding.get().is<Language::Field>()) {
      return static_cast<const Language::Field&>(binding.get());
    }
  }

  return {};
}

static auto select_structure(
    const Language::Types::Source& source,
    View::Bytes name) -> Option<const Language::Types::Structure&> {
  const Abstract& selected = source.resolve_context(name);
  BAIL_IF(!selected.is<Language::Types::Structure>());

  return static_cast<const Language::Types::Structure&>(selected);
}

PERIMORTEM_UNIT_TEST(
    LibraryTypeReference,
    authored_generic_types_keep_exact_recursive_identity) {
  static constexpr View::Bytes source =
      "// Generic TypeReference test.\n"
      "dialect : Library;\n"
      "public Catalog : struct {\n"
      "  public values : Access[Later];\n"
      "  public nested : View[Fixed[Unsigned_8, 4,],];\n"
      "  public repeated : View[Fixed[Unsigned_8, 4]];\n"
      "}\n"
      "public Later : struct { public state ready : Bool; }\n"
      "public Node : struct {\n"
      "  public state ready : Bool;\n"
      "  public children : View[Node];\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph =
      interpret(workspace, errors, "GenericTypeReference"_view, source);
  ASSERT(monograph);
  EXPECT(errors.is_empty());
  ASSERT(workspace.link(errors));
  EXPECT(errors.is_empty());

  const auto& root = monograph->get_source();
  auto catalog = select_structure(root, "Catalog"_view);
  auto later = select_structure(root, "Later"_view);
  auto node = select_structure(root, "Node"_view);
  ASSERT(catalog);
  ASSERT(later);
  ASSERT(node);

  auto values = find_field(*catalog, "values"_view);
  auto nested = find_field(*catalog, "nested"_view);
  auto repeated = find_field(*catalog, "repeated"_view);
  auto children = find_field(*node, "children"_view);
  ASSERT(values);
  ASSERT(nested);
  ASSERT(repeated);
  ASSERT(children);

  auto access = values->get_type().select<Language::Types::Access>();
  auto nested_view = nested->get_type().select<Language::Types::View>();
  auto repeated_view = repeated->get_type().select<Language::Types::View>();
  auto child_view = children->get_type().select<Language::Types::View>();
  ASSERT(access);
  ASSERT(nested_view);
  ASSERT(repeated_view);
  ASSERT(child_view);
  EXPECT(&access->get_element_type() == &*later);
  EXPECT(&nested->get_type() == &repeated->get_type());
  EXPECT(&child_view->get_element_type() == &*node);

  auto fixed = nested_view->get_element_type().select<Language::Types::Fixed>();
  ASSERT(fixed);
  EXPECT(&fixed->get_element_type() == &Dialect::get_unsigned_8());
  EXPECT_EQ(fixed->get_extent(), Unsigned_64(4));

  const Type* values_type = &values->get_type();
  const Type* nested_type = &nested->get_type();
  const Type* children_type = &children->get_type();

  // Repeating the transaction observes completed phases. Materializations and
  // each declaration retain the exact Types selected by the first pass.
  ASSERT(monograph->link());
  catalog = select_structure(root, "Catalog"_view);
  node = select_structure(root, "Node"_view);
  ASSERT(catalog);
  ASSERT(node);
  values = find_field(*catalog, "values"_view);
  nested = find_field(*catalog, "nested"_view);
  children = find_field(*node, "children"_view);
  ASSERT(values);
  ASSERT(nested);
  ASSERT(children);
  EXPECT(&values->get_type() == values_type);
  EXPECT(&nested->get_type() == nested_type);
  EXPECT(&children->get_type() == children_type);

  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
  EXPECT(
      &workspace.resolve_context("GenericTypeReference"_view) == &*monograph);
}

PERIMORTEM_UNIT_TEST(
    LibraryTypeReference,
    generic_arguments_complete_local_and_qualified_aliases) {
  static constexpr View::Bytes source =
      "// Generic Alias TypeReference test.\n"
      "dialect : Library;\n"
      "public LocalView : alias = View[LaterAlias];\n"
      "public QualifiedAccess : alias = Access[Container::NestedAlias];\n"
      "public LaterAlias : alias = Later;\n"
      "public Container : struct {\n"
      "  public NestedAlias : alias = Later;\n"
      "}\n"
      "public Later : struct { public state ready : Bool; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph =
      interpret(workspace, errors, "GenericAliasTypeReference"_view, source);
  ASSERT(monograph);
  EXPECT(errors.is_empty());
  ASSERT(workspace.link(errors));
  EXPECT(errors.is_empty());

  const auto& root = monograph->get_source();
  auto later = select_structure(root, "Later"_view);
  ASSERT(later);

  const Abstract& local_alias = root.resolve_context("LocalView"_view);
  const Abstract& qualified_alias =
      root.resolve_context("QualifiedAccess"_view);
  ASSERT(local_alias.is<Ttx::Model::Alias>());
  ASSERT(qualified_alias.is<Ttx::Model::Alias>());

  auto local_view = local_alias.resolve().select<Language::Types::View>();
  auto qualified_access =
      qualified_alias.resolve().select<Language::Types::Access>();
  ASSERT(local_view);
  ASSERT(qualified_access);
  EXPECT(&local_view->get_element_type() == &*later);
  EXPECT(&qualified_access->get_element_type() == &*later);

  // Completing an Alias reached from a Generic argument does not publish a
  // second generated identity. The two distinct formulas retain one canonical
  // materialization each, regardless of the Alias chain used to reach Later.
  EXPECT_EQ(monograph->get_materializations().get_size(), Count(2));
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    LibraryTypeReference,
    recursive_generic_alias_cycle_publishes_no_materialization) {
  static constexpr View::Bytes source =
      "// Recursive Generic Alias rejection.\n"
      "dialect : Library;\n"
      "public First : alias = View[Second];\n"
      "public Second : alias = View[First];"_view;
  Workspace workspace;
  Errors errors;
  auto monograph =
      interpret(workspace, errors, "RecursiveGenericAlias"_view, source);
  ASSERT(monograph);
  EXPECT(errors.is_empty());

  EXPECT_NOT(workspace.link(errors));
  EXPECT_NOT(errors.is_empty());
  EXPECT_EQ(monograph->get_materializations().get_size(), Count(0));

  auto diagnostics = monograph->get_diagnostics();
  ASSERT_NOT(diagnostics.is_empty());
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_message(),
      "Library Alias Type references contain a cycle."_view);
}

PERIMORTEM_UNIT_TEST(
    LibraryTypeReference,
    invalid_generic_application_fails_during_link) {
  struct Rejection {
    View::Bytes semantic_name;
    View::Bytes source;
    View::Bytes route;
  };
  static constexpr Static::Vector<Rejection, 3> rejections = {{
    Rejection{
      "ConcreteTypeArguments"_view,
      "// Concrete Type argument rejection.\n"
      "dialect : Library;\n"
      "public invalid : Unsigned_8[Unsigned_8];"_view,
      "Unsigned_8[Unsigned_8]"_view,
    },
    {
      "BareGeneric"_view,
      "// Bare Generic rejection.\n"
      "dialect : Library;\n"
      "public invalid : View;"_view,
      "View"_view,
    },
    {
      "EmptyGenericArguments"_view,
      "// Empty Generic argument rejection.\n"
      "dialect : Library;\n"
      "public invalid : View[];"_view,
      "View[]"_view,
    },
  }};

  for (Count i = 0; i < rejections.get_size(); i++) {
    const Rejection& rejection = rejections.get_data()[i];
    Workspace workspace;
    Errors errors;
    auto monograph =
        interpret(workspace, errors, rejection.semantic_name, rejection.source);
    ASSERT(monograph);
    EXPECT(errors.is_empty());
    EXPECT_NOT(workspace.link(errors));
    ASSERT_EQ(errors.get_size(), Count(1));

    auto diagnostics = monograph->get_diagnostics();
    ASSERT_EQ(diagnostics.get_size(), Count(1));
    const auto& diagnostic = diagnostics.get_data()[0];
    ASSERT(diagnostic.get_anchor());
    EXPECT_TEXT(
        diagnostic.get_anchor()->get_span().caculate_text(rejection.source),
        rejection.route);
    EXPECT_TEXT(
        diagnostic.get_message(),
        "Field Type route did not resolve to one stable Type."_view);
  }
}
