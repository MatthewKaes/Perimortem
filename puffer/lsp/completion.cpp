// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/completion.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "puffer/lsp/hover.hpp"
#include "puffer/lsp/semantic.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Puffer;
using namespace Tetrodotoxin::Library;

class Receiver {
 public:
  constexpr Receiver(
      const Language::Model::Type& type,
      Language::Model::Type::Access access)
      : type(type), access(access) {}

  constexpr auto get_type() const -> const Language::Model::Type& {
    return type.get();
  }

  constexpr auto get_access() const -> Language::Model::Type::Access {
    return access;
  }

 private:
  Ttx::Concept::Reference<const Language::Model::Type> type;
  Language::Model::Type::Access access;
};

static constexpr auto contains(Span span, Count offset) -> Bool {
  return span && offset >= span.get_offset() &&
         offset <= span.get_offset() + span.get_size();
}

static auto find_function(
    const Language::Types::Composite& composite,
    Count offset) -> Option<const Language::Function&> {
  for (const Reference<Abstract>& declaration : composite.get_declarations()) {
    auto function = declaration.get().select<Language::Function>();
    if (function) {
      auto body = function->get_body();
      if (body && contains(body->get_anchor().get_span(), offset)) {
        return *function;
      }
    }

    auto nested = declaration.get().select<Language::Types::Composite>();
    if (nested) {
      auto selected = find_function(*nested, offset);
      if (selected) {
        return selected;
      }
    }
  }
  return {};
}

static auto resolve_type_reference(
    const Language::TypeReference& reference,
    const Abstract& context) -> Option<const Language::Model::Type&> {
  Option<const Abstract&> selected;
  reference.resolve(context).visit(
      [&](const Abstract& resolved) { selected = resolved; },
      [](const Language::TypeReference::Failure&) {});
  BAIL_IF(!selected);
  auto direct = selected->select<Language::Model::Type>();
  return direct ? direct : selected->resolve().select<Language::Model::Type>();
}

static auto select_addressable_type(
    const Language::Model::Addressable& addressable,
    const Abstract& context) -> Option<const Language::Model::Type&> {
  auto field = addressable.select<Language::Field>();
  if (field) {
    if (field->is_linked()) {
      return field->get_type();
    }
    auto reference = field->get_type_reference();
    return reference ? resolve_type_reference(*reference, context)
                     : Option<const Language::Model::Type&>();
  }

  auto local = addressable.select<Language::Flow::Local>();
  if (local) {
    auto linked = local->get_linked_type();
    if (linked) {
      return linked;
    }
    auto reference = local->get_type_reference();
    return reference ? resolve_type_reference(*reference, context)
                     : Option<const Language::Model::Type&>();
  }

  const Abstract& resolved = addressable.resolve();
  auto completed = resolved.select<Language::Model::Addressable>();
  return completed ? Option<const Language::Model::Type&>(completed->get_type())
                   : Option<const Language::Model::Type&>();
}

static auto resolve_identifier(
    const Language::Expressions::Identifier& identifier,
    const Tetrodotoxin::Language::Monograph& monograph,
    const Language::Function* function) -> Option<Receiver> {
  View::Bytes name = identifier.get_name();
  const Abstract* selected = &identifier.resolve_authored();
  if (selected->is<Invalid>()) {
    selected = &monograph.resolve_context(name);
  }
  auto library = monograph.select<Language::Monograph>();
  if (library && selected->is<Invalid>()) {
    const Abstract& local = library->get_source().resolve_local(
        name, Tetrodotoxin::Language::Visibility::Private);
    if (!local.is<Invalid>()) {
      selected = &local;
    }
  }

  auto type = selected->select<Language::Model::Type>();
  if (type) {
    return Receiver(*type, Language::Model::Type::Access::Static);
  }
  auto addressable = selected->select<Language::Model::Addressable>();
  auto addressable_type =
      addressable
          ? select_addressable_type(
                *addressable,
                function ? static_cast<const Abstract&>(function->get_host())
                         : static_cast<const Abstract&>(monograph))
          : Option<const Language::Model::Type&>();
  return addressable_type
             ? Option<Receiver>(Receiver(
                   *addressable_type, Language::Model::Type::Access::Self))
             : Option<Receiver>();
}

static auto resolve_context_expression(
    const Language::Expression& expression,
    const Tetrodotoxin::Language::Monograph& monograph)
    -> Option<const Abstract&> {
  const Abstract& completed = expression.get_result();
  if (!completed.is<Invalid>()) {
    return completed.resolve();
  }

  auto identifier = expression.select<Language::Expressions::Identifier>();
  if (identifier) {
    const Abstract& authored = identifier->resolve_authored();
    if (!authored.is<Invalid>()) {
      return authored.resolve();
    }
    const Abstract& root = monograph.resolve_context(identifier->get_name());
    return root.is<Invalid>() ? Option<const Abstract&>()
                              : Option<const Abstract&>(root.resolve());
  }

  auto type_access = expression.select<Language::Access::Type>();
  if (type_access) {
    const Abstract& authored = type_access->resolve_authored();
    if (!authored.is<Invalid>()) {
      return authored.resolve();
    }
  }
  return {};
}

static auto resolve_expression(
    const Language::Expression& expression,
    const Tetrodotoxin::Language::Monograph& monograph,
    const Language::Function* function,
    Count offset,
    const Abstract& host) -> Option<Receiver> {
  const Abstract& result = expression.get_result();
  auto type = result.select<Language::Model::Type>();
  if (type) {
    return Receiver(*type, Language::Model::Type::Access::Static);
  }
  auto addressable = result.select<Language::Model::Addressable>();
  if (addressable) {
    auto selected = select_addressable_type(*addressable, host);
    if (selected) {
      return Receiver(*selected, Language::Model::Type::Access::Self);
    }
  }

  auto identifier = expression.select<Language::Expressions::Identifier>();
  if (identifier) {
    return resolve_identifier(*identifier, monograph, function);
  }

  auto address = expression.select<Language::Access::Address>();
  if (address) {
    auto receiver = resolve_expression(
        address->get_receiver(), monograph, function, offset, host);
    BAIL_IF(!receiver);
    const Abstract& selected = receiver->get_type().resolve_type_access(
        host, address->get_name(), receiver->get_access());
    auto selected_addressable = selected.select<Language::Model::Addressable>();
    auto selected_type =
        selected_addressable
            ? select_addressable_type(*selected_addressable, host)
            : Option<const Language::Model::Type&>();
    return selected_type
               ? Option<Receiver>(Receiver(
                     *selected_type, Language::Model::Type::Access::Self))
               : Option<Receiver>();
  }

  auto type_access = expression.select<Language::Access::Type>();
  if (type_access) {
    auto selected = resolve_context_expression(*type_access, monograph);
    BAIL_IF(!selected);
    auto selected_type = selected->select<Language::Model::Type>();
    return selected_type
               ? Option<Receiver>(Receiver(
                     *selected_type, Language::Model::Type::Access::Static))
               : Option<Receiver>();
  }

  auto output = expression.get_type().select<Language::Model::Type>();
  return output ? Option<Receiver>(
                      Receiver(*output, Language::Model::Type::Access::Self))
                : Option<Receiver>();
}

static auto completion_item(
    Allocator::Arena& arena,
    const Abstract& candidate,
    S64 kind) -> Json::Node {
  Json::Node hover = Lsp::semantic_hover(arena, candidate);
  return Json::Blueprint{
    {
      {"label"_view, candidate.get_name()},
      {"kind"_view, kind},
      {"documentation"_view, hover["contents"_view]},
    }}.construct(arena);
}

static auto complete_context_types(
    Allocator::Arena& arena,
    Managed::Vector<Json::Node>& items,
    const Abstract& context,
    auto candidates) -> void {
  for (const Reference<Abstract>& candidate : candidates) {
    const Abstract& selected =
        context.resolve_context(candidate.get().get_name());
    if (&selected == &candidate.get() ||
        &selected.resolve() == &candidate.get()) {
      items.insert(completion_item(arena, candidate.get(), S64(7)));
    }
  }
}

static auto complete_context(
    Allocator::Arena& arena,
    Managed::Vector<Json::Node>& items,
    const Abstract& context) -> void {
  auto composite = context.select<Language::Types::Composite>();
  if (composite) {
    complete_context_types(arena, items, context, composite->get_types());
    return;
  }

  auto monograph = context.select<Language::Monograph>();
  if (monograph) {
    complete_context_types(
        arena, items, context, monograph->get_source().get_types());
  }
}

static auto retains(
    Language::Model::Type::Callables bindings,
    const Abstract& candidate) -> Bool {
  for (const Reference<Abstract>& binding : bindings) {
    if (&binding.get() == &candidate) {
      return True;
    }
  }
  return False;
}

static auto complete_access(
    Allocator::Arena& arena,
    Managed::Vector<Json::Node>& items,
    const Receiver& receiver,
    Code::Type operation,
    const Abstract& host) -> void {
  const Language::Model::Type& type = receiver.get_type();
  if (operation == Code::Type::AddressOp) {
    auto composite = type.select<Language::Types::Composite>();
    if (!composite) {
      return;
    }
    for (const Reference<Abstract>& binding : composite->get_addressables()) {
      const Abstract& selected = type.resolve_type_access(
          host, binding.get().get_name(), receiver.get_access());
      auto pending = binding.get().select<Language::Model::Addressable>();
      auto caller = host.select<Language::Model::Type>();
      Bool accessible = composite->is_published(binding.get()) ||
                        (caller && caller->has_private_access_to(type));
      Bool progressive = selected.is<Invalid>() && pending && accessible &&
                         pending->supports_access(receiver.get_access());
      if (&selected == &binding.get() || progressive) {
        items.insert(completion_item(arena, binding.get(), S64(5)));
      }
    }
    return;
  }

  if (operation == Code::Type::TypeAccessOp) {
    auto composite = type.select<Language::Types::Composite>();
    if (!composite) {
      return;
    }
    for (const Reference<Abstract>& binding : composite->get_types()) {
      const Abstract& selected = type.resolve_context(binding.get().get_name());
      if (&selected == &binding.get()) {
        items.insert(completion_item(arena, binding.get(), S64(7)));
      }
    }
    return;
  }

  if (operation != Code::Type::CallOp) {
    return;
  }
  for (const Reference<Abstract>& binding : type.get_callables()) {
    const Abstract& selected = type.resolve_type_call(
        host, binding.get().get_name(), receiver.get_access());
    auto pending = binding.get().select<Language::Model::Callable>();
    auto caller = host.select<Language::Model::Type>();
    Bool published = retains(
        type.get_callables(Tetrodotoxin::Language::Visibility::Public),
        binding.get());
    Bool accessible =
        published || (caller && caller->has_private_access_to(type));
    Bool self = receiver.get_access() == Language::Model::Type::Access::Self;
    Bool progressive = selected.is<Invalid>() && pending && accessible &&
                       pending->declares_self() == self;
    if (&selected == &binding.get() || progressive) {
      items.insert(completion_item(arena, binding.get(), S64(3)));
    }
  }
}

static auto is_completion_suffix(View::Bytes source) -> Bool {
  for (Count index = 0; index < source.get_size(); index++) {
    U8 byte = source[index];
    Bool name = (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
                (byte >= '0' && byte <= '9') || byte == '_';
    if (!name && byte != ' ' && byte != '\t') {
      return False;
    }
  }
  return True;
}

auto Puffer::Lsp::completion(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  Allocator::Arena& arena = message.get_arena();
  const Json::Node params = message.get_params();
  View::Bytes uri =
      params["textDocument"_view]["uri"_view].decode_string(arena);
  const Json::Node line = params["position"_view]["line"_view];
  const Json::Node character = params["position"_view]["character"_view];
  Managed::Vector<Json::Node> items(arena);
  if (uri.is_empty() || !line.is_number() || !character.is_number() ||
      line.get_number() < 0 || character.get_number() < 0) {
    return message.report_result(Json::Node(items.get_view()));
  }

  View::Bytes source = documents.get_text(uri);
  auto offset = documents.get_position_encoding().find_offset(
      source, PositionEncoding::Position(
                  Count(line.get_number()), Count(character.get_number())));
  auto associations = documents.get_associations(uri);
  auto monograph = documents.get_monograph(uri);
  auto tokens = documents.get_tokens(uri);
  if (!offset || !associations || !monograph) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Count operation_index = Count(-1);
  for (Count index = 0; index < tokens.get_size(); index++) {
    Token token = tokens.get_data()[index];
    if (!token || Count(token.get_offset()) + token.get_size() > *offset) {
      continue;
    }
    Code::Type code = token.get_code().get_type();
    if (code == Code::Type::AddressOp || code == Code::Type::TypeAccessOp ||
        code == Code::Type::CallOp) {
      operation_index = index;
    }
  }
  if (operation_index == Count(-1) || operation_index == 0) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Token operation = tokens.get_data()[operation_index];
  Count operation_end = Count(operation.get_offset()) + operation.get_size();
  if (*offset < operation_end || !is_completion_suffix(source.slice(
                                     operation_end, *offset - operation_end))) {
    return message.report_result(Json::Node(items.get_view()));
  }
  Token receiver_token = tokens.get_data()[operation_index - 1];
  auto semantic = associations->find_at(
      Count(receiver_token.get_offset()) + receiver_token.get_size() - 1);
  if (!semantic) {
    return message.report_result(Json::Node(items.get_view()));
  }

  const Language::Monograph* library =
      monograph->select<Language::Monograph>()
          ? &static_cast<const Language::Monograph&>(*monograph)
          : nullptr;
  auto enclosing = library ? find_function(library->get_source(), *offset)
                           : Option<const Language::Function&>();
  const Language::Function* function = enclosing ? &*enclosing : nullptr;
  const Abstract& host =
      function ? static_cast<const Abstract&>(function->get_host())
               : static_cast<const Abstract&>(*monograph);
  auto expression = semantic->select<Language::Expression>();
  if (operation.get_code() == Code::Type::TypeAccessOp) {
    auto context = expression
                       ? resolve_context_expression(*expression, *monograph)
                       : Option<const Abstract&>();
    if (!context) {
      const Abstract& subject = semantic_subject(*semantic).resolve();
      if (!subject.is<Invalid>()) {
        context = subject;
      }
    }
    if (context) {
      complete_context(arena, items, *context);
    }
    return message.report_result(Json::Node(items.get_view()));
  }

  auto receiver =
      expression
          ? resolve_expression(*expression, *monograph, function, *offset, host)
          : Option<Receiver>();
  if (!receiver) {
    const Abstract& subject = semantic_subject(*semantic);
    auto type = subject.select<Language::Model::Type>();
    auto addressable = subject.select<Language::Model::Addressable>();
    auto selected_type = addressable
                             ? select_addressable_type(*addressable, host)
                             : Option<const Language::Model::Type&>();
    if (type) {
      receiver = Receiver(*type, Language::Model::Type::Access::Static);
    } else if (selected_type) {
      receiver = Receiver(*selected_type, Language::Model::Type::Access::Self);
    }
  }

  if (receiver) {
    complete_access(
        arena, items, *receiver, operation.get_code().get_type(), host);
  }
  return message.report_result(Json::Node(items.get_view()));
}
