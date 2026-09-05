// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/completion.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "puffer/lsp/hover.hpp"
#include "ttx/query.hpp"

using namespace Perimortem;
using namespace Perimortem::Serialization;
using namespace Puffer;
using namespace Ttx;

static auto name(ttx_abstract candidate) -> Core::View::Bytes {
  const ttx_borrowed_bytes value = candidate.operations->name(candidate);
  return Core::View::Bytes(value.data, value.size);
}

static auto query_concept(ttx_abstract candidate, Core::View::Bytes query)
    -> ttx_abstract {
  return Ttx::resolve_concept(
      candidate, ttx_borrowed_bytes{
                   .data = query.get_data(),
                   .size = query.get_size(),
                 });
}

static auto completion_item(
    Memory::Allocator::Arena& arena,
    ttx_abstract candidate) -> Json::Node {
  S64 kind = 6;
  const Ttx::CallableObservation callable = Ttx::resolve_callable(candidate);
  const ttx_interface_relation addressable =
      Ttx::relation(candidate, ttx_addressable_requirement());
  const Ttx::DomainObservation domain = Ttx::resolve_domain(candidate);
  if (callable.state == Ttx::Observation::Resolved) {
    kind = 3;
  } else if (
      addressable == TTX_INTERFACE_SATISFIED ||
      addressable == TTX_INTERFACE_EQUIVALENT) {
    kind = 5;
  } else if (
      domain.state == Ttx::Observation::Resolved &&
      ttx_abstract_same(domain.domain, Ttx::resolve(candidate))) {
    kind = 7;
  }

  Json::Node hover = Lsp::semantic_hover(arena, candidate);
  return Json::Blueprint{
    {
      {"label"_view, name(candidate)},
      {"kind"_view, kind},
      {"documentation"_view, hover["contents"_view]},
    }}.construct(arena);
}

static constexpr auto is_suffix(Core::View::Bytes source) -> Bool {
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

static auto select_authority(
    ttx_abstract authored,
    Ttx::Lexical::Code::Type operation) -> ttx_abstract {
  const ttx_interface_relation addressable =
      Ttx::relation(authored, ttx_addressable_requirement());
  Ttx::DomainObservation domain = Ttx::resolve_domain(authored);
  if ((addressable == TTX_INTERFACE_SATISFIED ||
       addressable == TTX_INTERFACE_EQUIVALENT) &&
      domain.state == Ttx::Observation::Resolved) {
    return query_concept(domain.domain, "instance"_view);
  }

  if (domain.state == Ttx::Observation::Resolved &&
      ttx_abstract_same(domain.domain, Ttx::resolve(authored))) {
    return query_concept(authored, "static"_view);
  }

  const ttx_abstract authored_static = query_concept(authored, "static"_view);
  if (!ttx_abstract_same(authored_static, ttx_unknown()) &&
      !ttx_abstract_same(authored_static, ttx_none())) {
    return authored_static;
  }

  const ttx_abstract represented = Ttx::resolve(authored);
  const ttx_interface_relation represented_addressable =
      Ttx::relation(represented, ttx_addressable_requirement());
  domain = Ttx::resolve_domain(represented);
  if ((represented_addressable == TTX_INTERFACE_SATISFIED ||
       represented_addressable == TTX_INTERFACE_EQUIVALENT) &&
      domain.state == Ttx::Observation::Resolved) {
    return query_concept(domain.domain, "instance"_view);
  }
  if (domain.state == Ttx::Observation::Resolved &&
      ttx_abstract_same(domain.domain, represented)) {
    return query_concept(represented, "static"_view);
  }

  const ttx_abstract selected_domain =
      domain.state == Ttx::Observation::Resolved ? domain.domain
                                                 : ttx_unknown();
  return operation == Ttx::Lexical::Code::Type::TypeAccessOp
             ? query_concept(selected_domain, "static"_view)
             : query_concept(selected_domain, "instance"_view);
}

static auto accepts(ttx_abstract candidate, Ttx::Lexical::Code::Type operation)
    -> Bool {
  const Ttx::CallableObservation callable = Ttx::resolve_callable(candidate);
  const ttx_interface_relation addressable =
      Ttx::relation(candidate, ttx_addressable_requirement());
  if (operation == Ttx::Lexical::Code::Type::CallOp) {
    return callable.state == Ttx::Observation::Resolved ? True : False;
  }
  if (operation == Ttx::Lexical::Code::Type::AddressOp) {
    return addressable == TTX_INTERFACE_SATISFIED ||
                   addressable == TTX_INTERFACE_EQUIVALENT
               ? True
               : False;
  }
  return callable.state != Ttx::Observation::Resolved &&
                 addressable != TTX_INTERFACE_SATISFIED &&
                 addressable != TTX_INTERFACE_EQUIVALENT
             ? True
             : False;
}

static auto complete(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Json::Node>& items,
    ttx_abstract authority,
    Ttx::Lexical::Code::Type operation) -> void {
  if (ttx_abstract_same(authority, ttx_unknown()) ||
      ttx_abstract_same(authority, ttx_none())) {
    return;
  }

  class Visitor {
   public:
    Visitor(
        Memory::Allocator::Arena& arena,
        Memory::Managed::Vector<Json::Node>& items,
        Ttx::Lexical::Code::Type operation)
        : operations{
            .header =
                {
                  .size = sizeof(ttx_concept_sink_ops),
                  .abi_major = TTX_ABI_MAJOR,
                  .abi_minor = TTX_ABI_MINOR,
                },
            .item = call,
            .completed = completed,
          },
          sink{
            .operations = &operations,
            .self = reinterpret_cast<ttx_concept_sink_self*>(this),
          },
          arena(arena),
          items(items),
          operation(operation) {}

    ttx_concept_sink_ops operations;
    ttx_concept_sink sink;

   private:
    static auto call(
        ttx_concept_sink sink,
        ttx_borrowed_bytes,
        ttx_abstract candidate) -> void {
      auto& self = *reinterpret_cast<Visitor*>(sink.self);
      if (accepts(candidate, self.operation)) {
        self.items.insert(completion_item(self.arena, candidate));
      }
    }

    static auto completed(ttx_concept_sink) -> void {}

    Memory::Allocator::Arena& arena;
    Memory::Managed::Vector<Json::Node>& items;
    Ttx::Lexical::Code::Type operation;
  } visitor(arena, items, operation);
  authority.operations->visit_concepts(authority, visitor.sink);
}

auto Puffer::Lsp::completion(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  Memory::Allocator::Arena& arena = message.get_arena();
  const Json::Node params = message.get_params();
  Core::View::Bytes uri =
      params["textDocument"_view]["uri"_view].decode_string(arena);
  const Json::Node line = params["position"_view]["line"_view];
  const Json::Node character = params["position"_view]["character"_view];
  Memory::Managed::Vector<Json::Node> items(arena);
  if (uri.is_empty() || !line.is_number() || !character.is_number() ||
      line.get_number() < 0 || character.get_number() < 0) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Core::View::Bytes source = documents.get_text(uri);
  auto offset = documents.get_position_encoding().find_offset(
      source, PositionEncoding::Position(
                  Count(line.get_number()), Count(character.get_number())));
  auto associations = documents.get_associations(uri);
  auto tokens = documents.get_tokens(uri);
  if (!offset || !associations) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Count operation_index = Count(-1);
  for (Count index = 0; index < tokens.get_size(); index++) {
    Ttx::Lexical::Token token = tokens.get_data()[index];
    if (!token || Count(token.get_offset()) + token.get_size() > *offset) {
      continue;
    }
    auto code = token.get_code().get_type();
    if (code == Ttx::Lexical::Code::Type::AddressOp ||
        code == Ttx::Lexical::Code::Type::TypeAccessOp ||
        code == Ttx::Lexical::Code::Type::CallOp) {
      operation_index = index;
    }
  }
  if (operation_index == Count(-1) || operation_index == 0) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Ttx::Lexical::Token operation = tokens.get_data()[operation_index];
  Count operation_end = Count(operation.get_offset()) + operation.get_size();
  if (*offset < operation_end ||
      !is_suffix(source.slice(operation_end, *offset - operation_end))) {
    return message.report_result(Json::Node(items.get_view()));
  }

  Ttx::Lexical::Token receiver = tokens.get_data()[operation_index - 1];
  auto semantic = associations->find_at(
      Count(receiver.get_offset()) + receiver.get_size() - 1);
  if (semantic) {
    const ttx_abstract authority = select_authority(
        semantic->get_handle(), operation.get_code().get_type());
    complete(arena, items, authority, operation.get_code().get_type());
  }
  return message.report_result(Json::Node(items.get_view()));
}
