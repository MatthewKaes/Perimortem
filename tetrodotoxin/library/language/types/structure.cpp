// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library::Language;

static const Layouts::Fluid incomplete_layout;

static auto parse_visibility(Cursor& cursor) -> Option<Visibility> {
  if (cursor.matches(Code::Type::Public)) {
    cursor.consume();
    return Visibility::Public;
  }
  if (cursor.matches(Code::Type::Private)) {
    cursor.consume();
    return Visibility::Private;
  }

  cursor.create_token_error(
      "Library Structure declarations require `public` or `private` "
      "visibility."_view);
  return {};
}

static auto contains_name(
    View::Vector<Field> fields,
    View::Vector<Reference<Function>> callables,
    View::Bytes name) -> Bool {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields.get_data()[i].get_name() == name) {
      return True;
    }
  }

  for (Count i = 0; i < callables.get_size(); i++) {
    if (callables.get_data()[i].get().get_name() == name) {
      return True;
    }
  }

  return False;
}

static auto exposes_private_structure(const Type& type) -> Bool {
  return type.visit<Types::Structure>(
      [](const Types::Structure& structure) {
        return structure.get_visibility() == Visibility::Private ? True : False;
      },
      [](const Abstract&) { return False; });
}

static auto callable_exposes_private_structure(const Function& function)
    -> Option<Anchor> {
  auto signature = function.get_signature();
  if (!signature) {
    return function.get_name_token()
               ? Option<Anchor>(Anchor::create(Span(function.get_name_token())))
               : Option<Anchor>();
  }

  for (Count i = 0; i < signature->get_parameter_size(); i++) {
    auto type = signature->get_parameter_type(i);
    if (type && exposes_private_structure(*type)) {
      return signature->get_parameter_type_anchor(i);
    }
  }
  for (Count i = 0; i < signature->get_result_size(); i++) {
    auto type = signature->get_result_type(i);
    if (type && exposes_private_structure(*type)) {
      return signature->get_result_type_anchor(i);
    }
  }

  return {};
}

Types::Structure::Structure(
    Allocator::Arena& domain,
    View::Bytes name,
    const Documentation& documentation,
    Visibility visibility,
    Tetrodotoxin::Language::Monograph& parent,
    Anchor anchor,
    Anchor name_anchor)
    : domain(domain),
      name(name),
      documentation(documentation),
      visibility(visibility),
      parent(parent),
      anchor(anchor),
      name_anchor(name_anchor),
      authored_fields(domain),
      fields(domain),
      public_fields(domain),
      callables(domain),
      public_callables(domain) {}

auto Types::Structure::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Tetrodotoxin::Language::Monograph& parent,
    Materializations& materializations) -> Option<Structure&> {
  // One outer branch keeps the caller fixed while nested Function transactions
  // reuse the same immutable stream. Only the complete declaration can join.
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  auto visibility = parse_visibility(transaction);
  if (!visibility) {
    return {};
  }

  Token name_token = transaction.require(
      Code::Type::Type,
      "Library Structures require an authored Type shaped name."_view);
  if (!name_token) {
    return {};
  }
  if (!transaction.require(
          Code::Type::Define,
          "Library Structure names require `:` before `struct`."_view)) {
    return {};
  }

  Token structure_token = transaction.require(
      Code::Type::Addressable,
      "Library Structure declarations require `struct`."_view);
  if (!structure_token || structure_token.caculate_text(
                              transaction.get_source_text()) != "struct"_view) {
    if (structure_token) {
      transaction.create_token_error(
          structure_token,
          "Library Structure declarations require `struct`."_view);
    }
    return {};
  }
  if (!transaction.require(
          Code::Type::ScopeStart,
          "Library Structure bodies require an opening `{`."_view)) {
    return {};
  }

  Managed::Vector<Field> parsed_fields(domain);
  Managed::Vector<Reference<Function>> parsed_callables(domain);

  // Fields remain compact source facts until link can create Addressables.
  // Callable grammar already has a real Function owner, so Structure retains
  // that exact identity instead of reproducing its Signature or body policy.
  while (!transaction.matches(Code::Type::ScopeEnd)) {
    if (transaction.matches(Code::Type::Terminal)) {
      transaction.create_token_error(
          "Library Structure body reached the end of source before `}`."_view);
      return {};
    }

    const Documentation& member_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(transaction);
    if (!transaction.matches(Code::Type::Public) &&
        !transaction.matches(Code::Type::Private)) {
      transaction.create_token_error(
          "Library Structure members require `public` or `private` "
          "visibility."_view);
      return {};
    }

    if (transaction.peek(1).get_code() == Code::Type::Func) {
      auto function = Function::reserve(
          domain, transaction, member_documentation, parent, materializations);
      if (!function) {
        return {};
      }
      if (contains_name(
              parsed_fields, parsed_callables, function->get_name())) {
        transaction.create_token_error(
            function->get_name_token(),
            "Duplicate field or Callable name in one Library Structure."_view);
        return {};
      }
      if (!function->complete(transaction)) {
        return {};
      }

      parsed_callables.insert(*function);
      continue;
    }

    auto field = Field::interpret(transaction, member_documentation);
    if (!field) {
      return {};
    }
    if (contains_name(parsed_fields, parsed_callables, field->get_name())) {
      transaction.create_token_error(
          field->get_anchor().get_token(),
          "Duplicate field or Callable name in one Library Structure."_view);
      return {};
    }

    parsed_fields.insert(*field);
  }

  Token closing = transaction.consume();
  View::Bytes name = name_token.caculate_text(transaction.get_source_text());
  Anchor structure_anchor =
      Anchor::create(structure_token, Span(opening, closing));
  Anchor structure_name_anchor = Anchor::create(Span(name_token));

  // Grammar has closed before the Type begins at its final Arena address.
  // Failed declarations therefore leave only unreachable nested allocations.
  Structure& structure = domain.construct_from<Structure>([&]() -> Structure {
    return Structure(
        domain, name, documentation, *visibility, parent, structure_anchor,
        structure_name_anchor);
  });

  // The complete private transaction owns every source decision. Publishing
  // those compact slots after the closing brace keeps rejected grammar from
  // leaving a partial Structure while the Functions keep their final identity.
  // Exact capacity keeps every Field stable once its Addressable borrows it.
  structure.authored_fields.reset(parsed_fields.get_size());
  for (Count i = 0; i < parsed_fields.get_size(); i++) {
    structure.authored_fields.insert(parsed_fields[i]);
  }
  for (Count i = 0; i < parsed_callables.get_size(); i++) {
    Function& function = parsed_callables[i].get();
    structure.callables.insert(function);
    if (function.get_visibility() == Visibility::Public) {
      structure.public_callables.insert(function);
    }
  }

  cursor.join(transaction);
  return structure;
}

auto Types::Structure::link_fields() -> Bool {
  if (stage >= Stage::FieldsLinked) {
    return True;
  }
  if (stage != Stage::Authored) {
    parent.report(
        anchor, "Structure fields cannot link from this lifecycle stage."_view,
        "Begin with the complete authored Structure declaration."_view);
    return False;
  }

  Bool failed = False;
  for (Count i = 0; i < authored_fields.get_size(); i++) {
    failed |= !authored_fields[i].link(domain, parent, parent);
  }

  if (failed) {
    return False;
  }

  // All Types settle before any field becomes a semantic edge. Structured can
  // then borrow one complete ordered inventory without a partial Layout or a
  // copied member record surviving a failed link.
  for (Count i = 0; i < authored_fields.get_size(); i++) {
    Field& field = authored_fields[i];
    auto addressable = field.get_addressable();
    if (!addressable) {
      parent.report(
          field.get_anchor(),
          "Linked Structure Field has no Addressable projection."_view,
          "Complete the exact Field Type before constructing its Layout."_view);
      return False;
    }

    fields.insert(*addressable);
    if (field.get_visibility() == Visibility::Public) {
      public_fields.insert(*addressable);
    }
  }

  layout = domain.construct<Layouts::Structured>(fields.get_view());
  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Structure::link_callable_signatures() -> Bool {
  if (stage >= Stage::CallableSignaturesLinked) {
    return True;
  }
  if (stage != Stage::FieldsLinked) {
    parent.report(
        anchor, "Structure Callable signatures require linked Fields."_view,
        "Complete the Structure Type before linking nested Callables."_view);
    return False;
  }

  Bool failed = False;
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !callables[i].get().link_signature();
  }

  if (failed) {
    return False;
  }

  stage = Stage::CallableSignaturesLinked;
  return True;
}

auto Types::Structure::link_callable_bodies() -> Bool {
  if (stage >= Stage::CallablesLinked) {
    return True;
  }
  if (stage != Stage::CallableSignaturesLinked) {
    parent.report(
        anchor, "Structure Callable bodies require linked signatures."_view,
        "Complete every nested signature before linking its body."_view);
    return False;
  }

  Bool failed = False;
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !callables[i].get().link_body();
  }

  if (failed) {
    return False;
  }

  stage = Stage::CallablesLinked;
  return True;
}

auto Types::Structure::finalize() -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }
  if (stage != Stage::CallablesLinked) {
    parent.report(
        anchor, "An incomplete Structure cannot enter finalization."_view,
        "Link every field and nested Callable before finalizing."_view);
    return False;
  }

  Bool failed = False;

  // Public member edges are the only Structure publication paths. Private
  // fields and Callables may keep exact local Structure Types without leaking
  // them through a consumer visible signature.
  for (Count i = 0; i < authored_fields.get_size(); i++) {
    const Field& field = authored_fields[i];
    auto type = field.get_type();
    if (field.get_visibility() != Visibility::Public || !type ||
        !exposes_private_structure(*type)) {
      continue;
    }

    parent.report(
        field.get_type_anchor(),
        "Public Structure field exposes a private local Structure Type."_view,
        "Keep the field private or publish its exact Structure Type."_view);
    failed = True;
  }
  for (Count i = 0; i < public_callables.get_size(); i++) {
    const Function& function = public_callables[i].get();
    auto exposure = callable_exposes_private_structure(function);
    if (!exposure) {
      continue;
    }

    parent.report(
        exposure,
        "Public Structure Callable exposes a private local Structure Type."_view,
        "Keep the Callable private or publish its exact Structure Type."_view);
    failed = True;
  }

  // Function retains its folding policy even when member publication fails.
  // Running every finalizer preserves independent diagnostics and cache state.
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !callables[i].get().finalize();
  }

  if (failed) {
    return False;
  }

  stage = Stage::Finalized;
  return True;
}

auto Types::Structure::resolve() const -> const Abstract& {
  if (stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Types::Structure::resolve_context(View::Bytes route) const
    -> const Abstract& {
  if (stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  for (Count i = 0; i < fields.get_size(); i++) {
    const Addressable& field = fields.at(i).get();
    if (field.get_name() == route) {
      return field;
    }
  }
  for (Count i = 0; i < callables.get_size(); i++) {
    const Function& function = callables.at(i).get();
    if (function.get_name() == route) {
      return function;
    }
  }

  return Invalid::get_invalid();
}

auto Types::Structure::get_layout() const -> const Layout& {
  return layout.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Layouts::Structured& selected) -> const Layout& {
        return selected;
      });
}

auto Types::Structure::get_fields() const
    -> View::Vector<Reference<const Addressable>> {
  return fields;
}

auto Types::Structure::get_public_fields() const
    -> View::Vector<Reference<const Addressable>> {
  return public_fields;
}

auto Types::Structure::get_callables() const
    -> View::Vector<Reference<Function>> {
  return callables;
}

auto Types::Structure::get_public_callables() const
    -> View::Vector<Reference<const Function>> {
  return public_callables;
}

auto Types::Structure::get_authored_fields() const -> View::Vector<Field> {
  return authored_fields;
}
