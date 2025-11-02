// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/type.hpp"
#include "concepts/narrow_resolver.hpp"

using namespace Tetrodotoxin::Language::Parser;
using namespace Perimortem::Concepts;

auto Type::parse(Context& ctx) -> std::optional<Type> {
  return std::nullopt;
  // static constexpr TablePair<std::string_view, Handler> data[] = {
  //     // Stack objects
  //     {"Byt", Handler::Byt},
  //     {"Enu", Handler::Enu},
  //     {"Flg", Handler::Flg},
  //     {"Int", Handler::Int},
  //     {"Num", Handler::Num},
  //     {"Str", Handler::Str},
  //     {"Vec", Handler::Vec},
  //     {"Any", Handler::Any},
  //     // Heap objects
  //     {"Text", Handler::Text},
  //     {"List", Handler::List},
  //     {"Dict", Handler::Dict},
  //     {"Func", Handler::Func}};

  // using type_resolver =
  //     NarrowResolver<Handler, array_size(data), data, 'A', 'Z'>;

  // static_assert(sizeof(type_resolver::sparse_table) <= 2400,
  //               "Type sparse table should be less than 2400 bytes. "
  //               "Use keywords only 8 characters or shorter.");

  // auto start_token = &ctx.current();

  // if (ctx.check_klass(Classifier::Type, start_token->klass)) {
  //   ctx.advance();
  //   return std::nullopt;
  // }

  // Type type;
  // type.name = start_token->data;
  // type.handler =
  //     type_resolver::find_or_default(type.name, Type::Handler::Defined);

  // auto token = &ctx.advance();

  // // Specalization requested.
  // if (token->klass == Classifier::IndexStart) {
  //   token = &ctx.advance();
  //   // Empty type specialization: List[]
  //   if (token->klass == Classifier::IndexEnd) {
  //     ctx.range_error(
  //         std::format("{} provided empty type specialization list.", type.name),
  //         *start_token, *token);
  //     ctx.advance();
  //     return std::nullopt;
  //   }

  //   type.parameters = std::make_unique<std::vector<Type>>();
  //   auto index_token = token;
  //   bool bad_parse = false;
  //   bool expect_type = true;

  //   while (expect_type) {
  //     std::optional<Type> specialization = Type::parse(ctx);

  //     token = &ctx.current();
  //     if (token->klass != Classifier::Seperator &&
  //         token->klass != Classifier::IndexEnd) {
  //       ctx.token_error(std::format(
  //           "Unexpected {} encountered during type specialization. "
  //           "Expected either {} or {}.",
  //           klass_name(token->klass), klass_name(Classifier::Seperator),
  //           klass_name(Classifier::IndexEnd)));

  //       const auto expected_symbols =
  //           Classifier::EndStatement | Classifier::ScopeStart |
  //           Classifier::ScopeEnd | Classifier::GroupStart |
  //           Classifier::GroupEnd;
  //       token = &ctx.advance_balanced(Classifier::IndexStart,
  //                                     Classifier::IndexEnd, expected_symbols);

  //       if (token->klass != Classifier::IndexEnd) {
  //         ctx.range_error(
  //             std::format("Type specialization is missing a closing {}",
  //                         klass_name(Classifier::IndexEnd)),
  //             *index_token, *token);
  //       } else {
  //         ctx.advance();
  //       }

  //       return std::nullopt;
  //     }

  //     if (specialization)
  //       type.parameters->push_back(std::move(*specialization));

  //     if (token->klass == Classifier::IndexEnd)
  //       expect_type = false;

  //     // Consume the seperator or index terminator.
  //     ctx.advance();
  //   }

  //   if (bad_parse)
  //     return std::nullopt;
  // }

  // if (!validate_type(ctx, start_token, token, type))
  //   return std::nullopt;

  // return type;
}

auto Type::validate_type(Context& ctx,
                         const Token* start_token,
                         const Token* token,
                         const Type& type) -> bool {
  // switch (type.handler) {
  //   case Handler::List:
  //   case Handler::Dict:
  //   case Handler::Func:
  //   case Handler::Vec:
  //     if (!type.parameters) {
  //       ctx.range_error(
  //           std::format("{} requires specialization to be used.", type.name),
  //           *start_token, *start_token);
  //       return false;
  //     } else if (type.handler == Handler::List &&
  //                type.parameters->size() != 1) {
  //       ctx.range_error(std::format("List takes 1 type parameter but got {}.",
  //                                   type.parameters->size()),
  //                       *start_token, *token);
  //       return false;
  //     } else if (type.handler == Handler::Dict) {
  //       // Dictionaries can have a few issues so try to provide as much useful
  //       // information in a single pass as possible.
  //       bool valid_dict = true;
  //       if (type.parameters->size() != 2) {
  //         ctx.range_error(
  //             std::format("Dict takes 2 type parameters but got {}.",
  //                         type.parameters->size()),
  //             *start_token, *token);
  //         valid_dict = false;
  //       }

  //       // Incorrect key type for Dict.
  //       if (type.parameters->size() > 0 &&
  //           type.parameters->at(0).handler == Handler::Defined) {
  //         ctx.range_error(std::format("{} can't be used as a Dict key. Defined "
  //                                     "types are only supported as values.",
  //                                     type.parameters->at(0).name),
  //                         *(start_token + 2), *(start_token + 2));
  //         valid_dict = false;
  //       }
  //       return valid_dict;
  //     }
  //     break;
  //   case Handler::Defined:
  //     if (type.parameters) {
  //       ctx.range_error(
  //           std::format(
  //               "Class `{}` can't be specialized. TTX does not support "
  //               "user defined generics. Only generic types (List, Dict, "
  //               "Func) can be specialized.",
  //               type.name),
  //           *start_token, *token);
  //       return false;
  //     }
  //     break;
  //   default:
  //     if (type.parameters) {
  //       ctx.range_error(
  //           std::format("Type `{}` can't be specialized. Only generic types "
  //                       "(List, Dict, Func) can be specialized.",
  //                       type.name),
  //           *start_token, *token);
  //       return false;
  //     }
  //     break;
  // }

  return true;
}