// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/dialect.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/environment/provider.h"
#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/import.hpp"
#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/language/production.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/package/archive/graph_import.hpp"
#include "tetrodotoxin/package/archive/writer.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/terminal/artifact_request.hpp"
#include "ttx/lexical/lexicon.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/query.hpp"
#include "ttx/model/layouts/value.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

class SelectedMember {
 public:
  constexpr SelectedMember(View::Bytes name, const Language::Monograph& value)
      : name(name), value(&value) {}

  View::Bytes name;
  const Language::Monograph* value;
};

class PackageProductionError final : public Tetrodotoxin::Language::Error {
 public:
  explicit PackageProductionError(View::Bytes message) : message(message) {}

  TTX_NAME("Package production error"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto describe(Ttx::Lexical::Errors::Report& report) const -> void override {
    report << message;
  }

 private:
  View::Bytes message;
};

static void production_failure(
    Allocator::Arena& arena,
    View::Bytes message,
    tetrodotoxin_production_result result) {
  auto& error = arena.construct<PackageProductionError>(arena.proxy(message));
  result.operations->failed(result.self, error.get_handle());
}

class MonographVisitor {
 public:
  explicit MonographVisitor(
      Managed::Vector<const Language::Monograph*>& monographs)
      : callable{&operations},
        operations{.call = call},
        monographs(monographs) {}

  ttx_named_abstract_callable callable;

 private:
  static auto call(
      ttx_named_abstract_callable* callable,
      ttx_borrowed_bytes,
      const ttx_abstract* abstract) -> void {
    auto& self = *reinterpret_cast<MonographVisitor*>(callable);
    auto monograph = Abstract::from_abi(abstract).select<Language::Monograph>();
    if (monograph) {
      self.monographs.insert(&*monograph);
    }
  }

  ttx_named_abstract_callable_operations operations;
  Managed::Vector<const Language::Monograph*>& monographs;
};

static auto find_monograph(
    View::Vector<const Language::Monograph*> graph,
    const Abstract& root) -> Option<const Language::Monograph&> {
  for (const Language::Monograph* retained : graph) {
    if (&retained->get_root() == &root) {
      return *retained;
    }
  }
  return {};
}

static auto find_member(
    View::Vector<SelectedMember> members,
    const Language::Monograph& monograph) -> Option<const SelectedMember&> {
  for (const SelectedMember& selected : members) {
    if (selected.value == &monograph) {
      return selected;
    }
  }
  return {};
}

static auto append_route(
    Allocator::Arena& arena,
    View::Bytes parent,
    View::Bytes child) -> View::Bytes {
  Managed::Bytes route(arena, parent);
  if (!parent.is_empty()) {
    route.concat("::"_view);
  }
  route.concat(child);
  return route.get_view();
}

static auto retain_members(
    Allocator::Arena& arena,
    View::Vector<const Language::Monograph*> graph,
    const Language::Monograph& importer,
    View::Bytes importer_name,
    Managed::Vector<SelectedMember>& members) -> Bool {
  for (const Language::Import* retained : importer.get_imports()) {
    const Language::Import& import = *retained;
    if (import.get_kind() != Language::Import::Kind::Source) {
      continue;
    }

    auto acquired = import.get_acquired();
    BAIL_IF(!acquired);
    auto selected = find_monograph(graph, *acquired);
    BAIL_IF(!selected);
    if (find_member(members.get_view(), *selected)) {
      continue;
    }

    View::Bytes route = append_route(
        arena,
        importer_name == "PackageSurface"_view ? View::Bytes() : importer_name,
        import.get_name());
    members.insert(SelectedMember(route, *selected));
    BAIL_IF(!retain_members(arena, graph, *selected, route, members));
  }
  return True;
}

static auto retain_imports(
    const Language::Monograph& importer,
    View::Bytes importer_name,
    View::Vector<SelectedMember> members,
    Managed::Vector<Package::Archive::GraphImport>& imports) -> Bool {
  for (const Language::Import* retained : importer.get_imports()) {
    const Language::Import& import = *retained;
    View::Bytes target = import.get_locator();
    if (import.get_kind() == Language::Import::Kind::Source) {
      auto acquired = import.get_acquired();
      BAIL_IF(!acquired);
      target = {};
      for (const SelectedMember& member : members) {
        if (&member.value->get_root() == &*acquired) {
          target = member.name;
          break;
        }
      }
      BAIL_IF(target.is_empty());
    }

    imports.insert(
        Package::Archive::GraphImport(
            importer_name, import.get_name(), import.get_visibility(),
            import.get_kind(), target, import.get_version(),
            import.get_route()));
  }
  return True;
}

static auto parse_quoted(Cursor& cursor) -> Option<View::Bytes> {
  Token token = cursor.require(
      Code::Type::String,
      "Package coordinates require quoted String values."_view);
  BAIL_IF(!token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, text)) {
    cursor.create_token_error(
        token, "Package coordinate String is unterminated."_view);
    return {};
  }
  return text.slice(1, text.get_size() - 2);
}

static auto require_spelling(
    Cursor& cursor,
    Code::Type code,
    View::Bytes spelling,
    View::Bytes message) -> Bool {
  Token token = cursor.require(code, message);
  BAIL_IF(!token);
  if (token.caculate_text(cursor.get_source_text()) != spelling) {
    cursor.create_token_error(token, message);
    return False;
  }
  return True;
}

static auto parse_named(Cursor& cursor, View::Bytes expected)
    -> Option<View::Bytes> {
  BAIL_IF(!cursor.require(
      Code::Type::AddressOp,
      "Package coordinate arguments require a leading `.`."_view));
  Token name = cursor.require(
      Code::Type::Addressable,
      "Package coordinate arguments require a named slot."_view);
  BAIL_IF(!name);
  if (name.caculate_text(cursor.get_source_text()) != expected) {
    cursor.create_token_error(
        name, "Package coordinate argument is out of order."_view);
    return {};
  }
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Package coordinate named arguments require `=`."_view));
  return parse_quoted(cursor);
}

static auto parse_coordinate(
    Cursor& cursor,
    View::Bytes& identity,
    Perimortem::System::Version& version) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::Package,
      "Package sources require `package(.name = ..., .version = ...);`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "Package coordinates require `(` before their arguments."_view));
  auto name = parse_named(cursor, "name"_view);
  BAIL_IF(!name);
  BAIL_IF(!cursor.require(
      Code::Type::PackingOp,
      "Package coordinates require both `name` and `version`."_view));
  auto version_text = parse_named(cursor, "version"_view);
  BAIL_IF(!version_text);
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd,
      "Package coordinates require `)` after their arguments."_view));
  BAIL_IF(!cursor.require(
      Code::Type::EndStatement,
      "Package coordinates require one terminating `;`."_view));

  version = Perimortem::System::Version::parse(*version_text);
  if (name->is_empty() || version.is_null()) {
    cursor.create_token_error(
        "Package coordinates require a nonempty name and canonical Major.Minor version."_view);
    return False;
  }
  identity = cursor.get_arena().proxy(*name);
  return True;
}

static auto parse_required_dialect(Cursor& cursor)
    -> Option<Package::RequiredDialect> {
  BAIL_IF(!cursor.require(
      Code::Type::Package,
      "Package Dialect requirements begin with `package`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "Package Dialect coordinate requires `(` before its fields."_view));
  auto name = parse_named(cursor, "name"_view);
  BAIL_IF(!name || name->is_empty());
  BAIL_IF(!cursor.require(
      Code::Type::PackingOp,
      "Package Dialect coordinate requires both name and version."_view));
  auto version_text = parse_named(cursor, "version"_view);
  BAIL_IF(!version_text);
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd,
      "Package Dialect coordinate requires `)` after its fields."_view));
  BAIL_IF(!cursor.require(
      Code::Type::TypeAccessOp,
      "Package Dialect coordinate requires one exported route."_view));
  Token route = cursor.require(
      Code::Type::Type,
      "Package Dialect coordinate requires one Type-shaped export name."_view);
  BAIL_IF(!route);
  const Perimortem::System::Version version =
      Perimortem::System::Version::parse(*version_text);
  if (version.is_null()) {
    cursor.create_token_error(
        route,
        "Package Dialect version must use canonical Major.Minor form."_view);
    return {};
  }
  return Package::RequiredDialect{
    .package = cursor.get_arena().proxy(*name),
    .version = version,
    .route =
        cursor.get_arena().proxy(route.caculate_text(cursor.get_source_text())),
  };
}

auto Package::Dialect::parse_requirements(Cursor& cursor)
    -> Option<Managed::Vector<RequiredDialect>> {
  Managed::Vector<RequiredDialect> requirements(cursor.get_arena());
  if (cursor.get_code() != Code::Type::Addressable ||
      cursor.get_text() != "requires"_view) {
    return requirements;
  }
  cursor.consume();
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "requires needs `(` before its named fields."_view));
  BAIL_IF(!cursor.require(
      Code::Type::AddressOp, "requires fields begin with `.`."_view));
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Addressable, "dialects"_view,
      "requires currently accepts the `.dialects` field."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "requires `.dialects` needs `=` before its list."_view));
  BAIL_IF(!cursor.require(
      Code::Type::BracketStart,
      "requires `.dialects` needs an opening `[` list."_view));
  while (!cursor.matches(Code::Type::BracketEnd)) {
    auto requirement = parse_required_dialect(cursor);
    BAIL_IF(!requirement);
    for (const RequiredDialect& existing : requirements.get_view()) {
      if (existing.package == requirement->package &&
          existing.version == requirement->version &&
          existing.route == requirement->route) {
        cursor.create_token_error(
            "Package repeats one exact Dialect requirement."_view);
        return {};
      }
    }
    requirements.insert(*requirement);
    if (!cursor.matches(Code::Type::PackingOp)) {
      break;
    }
    cursor.consume();
  }
  BAIL_IF(!cursor.require(
      Code::Type::BracketEnd,
      "requires `.dialects` needs a closing `]`."_view));
  if (cursor.matches(Code::Type::PackingOp)) {
    cursor.consume();
  }
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd, "requires needs `)` after its fields."_view));
  BAIL_IF(!cursor.require(
      Code::Type::EndStatement, "requires needs a terminating `;`."_view));
  return requirements;
}

auto Package::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  Allocator::Arena& transaction = cursor.get_arena();
  Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  auto requirements = parse_requirements(cursor);
  BAIL_IF(!requirements);
  Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  View::Bytes identity;
  Perimortem::System::Version version;
  BAIL_IF(!parse_coordinate(cursor, identity, version));

  Managed::Vector<Tetrodotoxin::Language::Import::Description> imports(
      transaction);
  while (Tetrodotoxin::Language::Parser::Import::is_next(cursor)) {
    const Documentation& import_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto import = Tetrodotoxin::Language::Parser::Import::parse(
        cursor, import_documentation);
    if (import) {
      imports.insert(*import);
    } else {
      cursor.recover_to_statement();
    }
  }

  auto& monograph = Language::Monograph::create_authored(
      transaction, *this, documentation, source_anchor, identity, version,
      context, library, *requirements);
  auto& root = monograph.edit_library().get_source();
  while (!cursor.matches(Code::Type::Terminal)) {
    const Documentation& declaration_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto definition = Tetrodotoxin::Language::Definition::parse(
        cursor, declaration_documentation, root);
    if (!definition) {
      cursor.recover_to_statement();
      continue;
    }
    auto member = Library::Interpreter::Member::parse(cursor, *definition);
    if (!member || member->get_category() !=
                       Library::Language::Types::Composite::Category::Type) {
      cursor.create_expression_error(
          definition->get_anchor(),
          "Package sources contain only Library Type definitions."_view,
          "Import source or Package roots with Alias declarations, then publish Types or namespaces."_view);
      cursor.recover_to_statement();
      continue;
    }
    root.retain_authored_definition(
        member->get_semantic(), *definition, member->get_category(), cursor,
        member->get_completion());
    if (member->needs_recovery()) {
      cursor.recover_to_statement();
    }
  }
  for (const Tetrodotoxin::Language::Import::Description& import :
       imports.get_view()) {
    if (!monograph.retain_import(import, cursor.get_associations())) {
      cursor.create_expression_error(
          import.get_declaration_anchor(),
          "Source repeats one local Import Type name."_view,
          "Give each imported source or Package one distinct local name."_view);
    }
  }
  return monograph;
}

void Package::Dialect::produce(
    ttx_context,
    Allocator::Arena& arena,
    tetrodotoxin_workspace_view workspace,
    const Tetrodotoxin::Language::Monograph& monograph,
    tetrodotoxin_production_result result) const {
  auto package = monograph.select<Language::Monograph>();
  if (!package || &package->resolve() != &*package ||
      workspace.operations == nullptr || workspace.self == nullptr ||
      workspace.operations->root == nullptr) {
    production_failure(
        arena,
        "Package production requires one completed Package Monograph."_view,
        result);
    return;
  }
  const ttx_abstract graph_handle = workspace.operations->root(workspace.self);
  auto graph = Abstract::from_handle(graph_handle);
  if (!graph) {
    production_failure(
        arena, "Package production requires its owning Workspace."_view,
        result);
    return;
  }

  Managed::Vector<const Tetrodotoxin::Language::Monograph*> graph_monographs(
      arena);
  MonographVisitor graph_visitor(graph_monographs);
  graph->visit_concepts(&graph_visitor.callable);
  Managed::Vector<SelectedMember> members(arena);
  if (!retain_members(
          arena, graph_monographs.get_view(), *package, "PackageSurface"_view,
          members)) {
    production_failure(
        arena, "Package production could not discover its source graph."_view,
        result);
    return;
  }

  Managed::Vector<Archive::Writer::GraphMember> archive_members(arena);
  Managed::Vector<Archive::GraphImport> imports(arena);
  for (const SelectedMember& selected : members.get_view()) {
    archive_members.insert(
        Archive::Writer::GraphMember(selected.name, *selected.value));
  }
  if (!retain_imports(
          *package, "PackageSurface"_view, members.get_view(), imports)) {
    production_failure(
        arena, "Package production could not retain its root imports."_view,
        result);
    return;
  }
  for (const SelectedMember& selected : members.get_view()) {
    if (!retain_imports(
            *selected.value, selected.name, members.get_view(), imports)) {
      production_failure(
          arena, "Package production could not retain a member import."_view,
          result);
      return;
    }
  }

  auto archive = Archive::Writer::write(
      *package, package->get_name(), package->get_version(),
      archive_members.get_view(), imports.get_view());
  if (!archive) {
    production_failure(
        arena,
        "Package production could not encode its completed source graph."_view,
        result);
    return;
  }

  Managed::Bytes publication_root(arena);
  Perimortem::Serialization::Stream::Textual<Managed::Bytes> output(
      publication_root);
  output << package->get_name() << "/"_view
         << package->get_version().get_major() << "."_view
         << package->get_version().get_minor();
  const View::Bytes archive_bytes = archive->get_view();
  auto request = Terminal::ArtifactRequest::create(
      std::vector<uint8_t>{
        'p', 'a', 'c', 'k', 'a', 'g', 'e', '.', 't', 't', 'x', 'p'},
      std::vector<uint8_t>(
          archive_bytes.get_data(),
          archive_bytes.get_data() + archive_bytes.get_size()),
      false, workspace);
  if (!request) {
    production_failure(
        arena, "Package production could not retain its archive request."_view,
        result);
    return;
  }
  const View::Bytes root = publication_root.get_view();
  auto production = Tetrodotoxin::Language::Production::create(
      std::vector<uint8_t>(root.get_data(), root.get_data() + root.get_size()),
      std::vector<tetrodotoxin_product_request>{*request});
  if (!production) {
    production_failure(
        arena, "Package production could not retain its publication plan."_view,
        result);
    return;
  }
  result.operations->planned(result.self, *production);
}
