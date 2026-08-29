// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/completion.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "puffer/lsp/hover.hpp"
#include "ttx/concept/none.h"
#include "ttx/concept/unknown.h"
#include "ttx/model/addressable.h"
#include "ttx/model/callable.h"
#include "ttx/model/type.h"

using namespace Perimortem;
using namespace Perimortem::Serialization;
using namespace Puffer;
using namespace Ttx;

static auto name(const ttx_abstract* candidate) -> Core::View::Bytes {
  perimortem_bytes value = ttx_abstract_name(candidate);
  return Core::View::Bytes(value.data, value.size);
}

static auto query_concept(
    const ttx_abstract* candidate,
    Core::View::Bytes query) -> const ttx_abstract* {
  return ttx_abstract_resolve_concept(
      candidate,
      perimortem_bytes{.data = query.get_data(), .size = query.get_size()});
}

static auto completion_item(
    Memory::Allocator::Arena& arena,
    const ttx_abstract* candidate) -> Json::Node {
  S64 kind = 6;
  ttx_callable_view callable;
  ttx_addressable_view addressable;
  ttx_type_view type;
  if (ttx_callable_prove(candidate, &callable)) {
    kind = 3;
  } else if (ttx_addressable_prove(candidate, &addressable)) {
    kind = 5;
  } else if (ttx_type_prove(candidate, &type)) {
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
    const ttx_abstract* authored,
    Ttx::Lexical::Code::Type operation) -> const ttx_abstract* {
  ttx_addressable_view addressable;
  if (ttx_addressable_prove(authored, &addressable)) {
    return query_concept(ttx_addressable_type(&addressable), "instance"_view);
  }

  ttx_type_view type;
  if (ttx_type_prove(authored, &type)) {
    return query_concept(authored, "static"_view);
  }

  const ttx_abstract* authored_static = query_concept(authored, "static"_view);
  ttx_unknown_view unknown;
  ttx_none_view none;
  if (!ttx_unknown_prove(authored_static, &unknown) &&
      !ttx_none_prove(authored_static, &none)) {
    return authored_static;
  }

  const ttx_abstract* represented = ttx_abstract_resolve(authored);
  if (ttx_addressable_prove(represented, &addressable)) {
    return query_concept(ttx_addressable_type(&addressable), "instance"_view);
  }
  if (ttx_type_prove(represented, &type)) {
    return query_concept(represented, "static"_view);
  }

  const ttx_abstract* selected_type =
      ttx_abstract_resolve(ttx_abstract_type(authored));
  return operation == Ttx::Lexical::Code::Type::TypeAccessOp
             ? query_concept(selected_type, "static"_view)
             : query_concept(selected_type, "instance"_view);
}

static auto accepts(
    const ttx_abstract* candidate,
    Ttx::Lexical::Code::Type operation) -> Bool {
  ttx_callable_view callable;
  ttx_addressable_view addressable;
  if (operation == Ttx::Lexical::Code::Type::CallOp) {
    return Bool(ttx_callable_prove(candidate, &callable));
  }
  if (operation == Ttx::Lexical::Code::Type::AddressOp) {
    return Bool(ttx_addressable_prove(candidate, &addressable));
  }
  return Bool(
      !ttx_callable_prove(candidate, &callable) &&
      !ttx_addressable_prove(candidate, &addressable));
}

static auto complete(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Json::Node>& items,
    const ttx_abstract* authority,
    Ttx::Lexical::Code::Type operation) -> void {
  ttx_unknown_view unknown;
  ttx_none_view none;
  if (ttx_unknown_prove(authority, &unknown) ||
      ttx_none_prove(authority, &none)) {
    return;
  }

  class Visitor {
   public:
    Visitor(
        Memory::Allocator::Arena& arena,
        Memory::Managed::Vector<Json::Node>& items,
        Ttx::Lexical::Code::Type operation)
        : callable{&operations},
          operations{.call = call},
          arena(arena),
          items(items),
          operation(operation) {}

    ttx_named_abstract_callable callable;

   private:
    static auto call(
        ttx_named_abstract_callable* callable,
        perimortem_bytes,
        const ttx_abstract* candidate) -> void {
      auto& self = *reinterpret_cast<Visitor*>(callable);
      if (accepts(candidate, self.operation)) {
        self.items.insert(completion_item(self.arena, candidate));
      }
    }

    ttx_named_abstract_callable_operations operations;
    Memory::Allocator::Arena& arena;
    Memory::Managed::Vector<Json::Node>& items;
    Ttx::Lexical::Code::Type operation;
  } visitor(arena, items, operation);
  ttx_abstract_visit_concepts(authority, &visitor.callable);
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
    const ttx_abstract* authority =
        select_authority(semantic->get_abi(), operation.get_code().get_type());
    complete(arena, items, authority, operation.get_code().get_type());
  }
  return message.report_result(Json::Node(items.get_view()));
}
