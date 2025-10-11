// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/ast/type.hpp"

#include <sstream>

using namespace Tetrodotoxin::Language::Parser;

auto Type::parse(Context& ctx) -> std::optional<Type> {
  auto start_token = &ctx.current();

  if (start_token->klass != Classifier::Type) {
    TTX_EXPECTED_CLASS_ERROR(Classifier::Type, start_token->klass);
    ctx.advance();
    return std::nullopt;
  }

  Type type;
  type.name = start_token->to_string();
  type.handler = detect_handler(type.name);

  auto token = &ctx.advance();

  // Specalization requested.
  if (token->klass == Classifier::IndexStart) {
    token = &ctx.advance();
    // Empty type specialization: List[]
    if (token->klass == Classifier::IndexEnd) {
      std::stringstream __details;
      __details << type.name << " provided empty type specialization list.";
      ctx.range_error(__details.view(), *start_token, *token);
      ctx.advance();
      return std::nullopt;
    }

    type.parameters = std::make_unique<std::vector<Type>>();
    auto index_token = token;
    bool bad_parse = false;
    bool expect_type = true;

    while (expect_type) {
      std::optional<Type> specialization = Type::parse(ctx);

      token = &ctx.current();
      if (token->klass != Classifier::Seperator &&
          token->klass != Classifier::IndexEnd) {
        std::stringstream __details;
        __details
            << "Unexpected `" << klass_name(token->klass)
            << "` encountered during type specialization. Expected either a "
            << klass_name(Classifier::Seperator) << " or "
            << klass_name(Classifier::IndexEnd) << " .";
        ctx.token_error(__details.view());

        const auto expected_symbols =
            Classifier::EndStatement | Classifier::ScopeStart |
            Classifier::ScopeEnd | Classifier::GroupStart |
            Classifier::GroupEnd;
        token = &ctx.advance_balanced(Classifier::IndexStart,
                                      Classifier::IndexEnd, expected_symbols);

        if (token->klass != Classifier::IndexEnd) {
          std::string __details = "Type specialization is missing a closing ";
          __details += klass_name(Classifier::IndexEnd);
          ctx.range_error(__details, *index_token, *token);
        } else {
          ctx.advance();
        }

        return std::nullopt;
      }

      if (specialization)
        type.parameters->push_back(std::move(*specialization));

      if (token->klass == Classifier::IndexEnd)
        expect_type = false;

      // Consume the seperator or index terminator.
      ctx.advance();
    }

    if (bad_parse)
      return std::nullopt;
  }

  if (!validate_type(ctx, start_token, token, type))
    return std::nullopt;

  return type;
}

auto Type::validate_type(Context& ctx,
                         const Token* start_token,
                         const Token* token,
                         const Type& type) -> bool {
  switch (type.handler) {
    case Handler::List:
    case Handler::Dict:
    case Handler::Func:
    case Handler::Vec:
      if (!type.parameters) {
        std::stringstream __details;
        __details << type.name << " requires specialization to be used.";
        switch (type.handler) {
          case Handler::List:
            __details << " Usage: List[ContentType], e.g: List[Float]";
            break;
          case Handler::Dict:
            __details << " Usage: Dict[KeyType, ValueType], e.g: Dict[String, "
                         "String]";
            break;
          case Handler::Func:
            __details << "Usage: Func[ReturnType, ParameterType...], e.g: "
                         "Func[Byt, String, Int]";
            break;
          case Handler::Vec:
            __details << "Usage: Vec[size:Int], e.g: "
                         "Vec[size:Int]";
            break;
          default:
            break;
        }
        ctx.range_error(__details.view(), *start_token, *token);
        return false;
      } else if (type.handler == Handler::List &&
                 type.parameters->size() != 1) {
        std::stringstream __details;
        __details << type.name << " takes 1 type parameter but "
                  << type.parameters->size() << " were provided.";
        ctx.range_error(__details.view(), *start_token, *token);
        return false;
      } else if (type.handler == Handler::Dict) {
        // Dictionaries can have a few issues so try to provide as much useful
        // information in a single pass as possible.
        bool valid_dict = true;
        if (type.parameters->size() != 2) {
          std::stringstream __details;
          __details << type.name << " takes 2 type parameters but "
                    << type.parameters->size();
          __details << (type.parameters->size() == 1 ? " was provided."
                                                     : " were provided.");
          ctx.range_error(__details.view(), *start_token, *token);
          valid_dict = false;
        }

        // Incorrect key type for Dict.
        if (type.parameters->size() > 0 &&
            type.parameters->at(0).handler == Handler::Defined) {
          std::stringstream __details;
          __details << type.parameters->at(0).name
                    << " can't be used as a Dict key. Defined types are only "
                       "supported as values.";
          ctx.range_error(__details.view(), *(start_token + 2),
                          *(start_token + 2));
          valid_dict = false;
        }
        return valid_dict;
      }
      break;
    case Handler::Defined:
      if (type.parameters) {
        std::stringstream __details;
        __details << "Class `" << type.name
                  << "` can't be specialized. TTX does not support user "
                     "defined generics. Only generic types "
                     "(List, Dict, Func) can be specialized.";
        ctx.range_error(__details.view(), *start_token, *token);
        return false;
      }
      break;
    default:
      if (type.parameters) {
        std::stringstream __details;
        __details << "Type `" << type.name
                  << "` can't be specialized. Only generic types (List, Dict, "
                     "Func) can be specialized.";
        ctx.range_error(__details.view(), *start_token, *token);
        return false;
      }
      break;
  }

  return true;
}