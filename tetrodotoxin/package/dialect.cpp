// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/dialect.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/import.hpp"
#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/package/archive/graph_import.hpp"
#include "tetrodotoxin/package/archive/writer.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/lexical/lexicon.hpp"
#include "ttx/model/context.h"
#include "ttx/model/layouts/named.h"
#include "ttx/model/layouts/value.h"

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
      perimortem_bytes,
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

auto Package::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  Allocator::Arena& transaction = cursor.get_arena();
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
      context, library);
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
        member->get_semantic(), *definition, member->get_category(), cursor);
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

auto Package::Dialect::produce(
    Allocator::Arena& arena,
    const Abstract& graph,
    const Tetrodotoxin::Language::Monograph& monograph) const
    -> const ttx_pack* {
  auto package = monograph.select<Language::Monograph>();
  if (!package || &package->resolve() != &*package) {
    Perimortem::Core::Diagnostics::Log::error(
        "Package production requires one completed Package Monograph."_view);
    return nullptr;
  }

  Managed::Vector<const Tetrodotoxin::Language::Monograph*> graph_monographs(
      arena);
  MonographVisitor graph_visitor(graph_monographs);
  ttx_abstract_visit_concepts(graph.get_abi(), &graph_visitor.callable);
  Managed::Vector<SelectedMember> members(arena);
  if (!retain_members(
          arena, graph_monographs.get_view(), *package, "PackageSurface"_view,
          members)) {
    Perimortem::Core::Diagnostics::Log::error(
        "Package production could not discover its source graph."_view);
    return nullptr;
  }

  Managed::Vector<Archive::Writer::GraphMember> archive_members(arena);
  Managed::Vector<Archive::GraphImport> imports(arena);
  for (const SelectedMember& selected : members.get_view()) {
    archive_members.insert(
        Archive::Writer::GraphMember(selected.name, *selected.value));
  }
  if (!retain_imports(
          *package, "PackageSurface"_view, members.get_view(), imports)) {
    Perimortem::Core::Diagnostics::Log::error(
        "Package production could not retain its root imports."_view);
    return nullptr;
  }
  for (const SelectedMember& selected : members.get_view()) {
    if (!retain_imports(
            *selected.value, selected.name, members.get_view(), imports)) {
      Perimortem::Core::Diagnostics::Log::error(
          "Package production could not retain a member import."_view);
      return nullptr;
    }
  }

  auto archive = Archive::Writer::write(
      *package, package->get_name(), package->get_version(),
      archive_members.get_view(), imports.get_view());
  if (!archive) {
    Perimortem::Core::Diagnostics::Log::error(
        "Package production could not encode its completed source graph."_view);
    return nullptr;
  }

  Managed::Bytes path(arena);
  Perimortem::Serialization::Stream::Textual<Managed::Bytes> output(path);
  output << package->get_name() << "/"_view
         << package->get_version().get_major() << "."_view
         << package->get_version().get_minor() << "/package.ttxp"_view;
  Tetrodotoxin::Language::Product& product =
      Tetrodotoxin::Language::Product::create(
          arena, path.get_view(), archive->get_view());
  ttx_value_layout product_value_layout;
  ttx_named_layout product_layout;
  perimortem_bytes product_name = {
    .data = path.get_view().get_data(),
    .size = path.get_view().get_size(),
  };
  ttx_value_layout_initialize(&product_value_layout, product.get_abi());
  ttx_named_layout_initialize(
      &product_layout, &product_value_layout.layout, &product_name, 1);
  auto product_packs = arena.reserve<ttx_model_pack>(1);
  auto product_layouts = arena.reserve<ttx_fluid_layout>(1);
  auto product_named_layouts = arena.reserve<ttx_named_layout>(1);
  auto product_entries = arena.reserve<const ttx_abstract*>(1);
  auto product_names = arena.reserve<perimortem_bytes>(1);
  auto name_storage = arena.allocate(path.get_size());
  ttx_model_context product_context;
  ttx_model_context_initialize(
      &product_context, ttx_model_context_storage{
                          .packs = product_packs.get_data(),
                          .layouts = product_layouts.get_data(),
                          .named_layouts = product_named_layouts.get_data(),
                          .entries = product_entries.get_data(),
                          .names = product_names.get_data(),
                          .name_bytes = name_storage.get_data(),
                          .pack_capacity = 1,
                          .entry_capacity = 1,
                          .name_capacity = 1,
                          .name_byte_capacity = path.get_size(),
                        });
  return ttx_context_pack(&product_context.context, &product_layout.layout);
}
