// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/script.hpp"
#include "parser/attribute.hpp"
#include "parser/comment.hpp"
#include "parser/context.hpp"

#include "types/library.hpp"

#include <filesystem>

using namespace Tetrodotoxin::Language::Parser;

auto create_library(Types::Program& host, Context& ctx, std::string& doc)
    -> std::unique_ptr<Types::Library> {
  auto token = &ctx.current();
  if (ctx.check_klass(Classifier::Package, token->klass))
    return std::make_unique<Types::Library>(host, doc, ctx.disk_path, false);

  bool is_entity = false;
  token = &ctx.advance();
  if (token->klass == Classifier::Entity) {
    is_entity = true;
  } else if (token->klass != Classifier::Library) {
    ctx.token_error(std::format(
        "Unknown package type `{}`, defaulting to `library`.", token->data));
  };

  token = &ctx.advance();

  // If we correctly have an end statement then consume it, otherwise report a
  // non-fatal token class error.
  if (!ctx.check_klass(Classifier::EndStatement, token->klass)) {
    // Eat end statement.
    token = &ctx.advance();
  }

  return std::make_unique<Types::Library>(host, doc, ctx.disk_path, is_entity);
}

auto Script::parse(Types::Program& host,
                   Errors& errors,
                   const std::filesystem::path& source_map,
                   Tokenizer& tokenizer) -> Types::Library* {
  Context ctx(source_map, tokenizer, errors);

  // Documentation | Attributes
  auto token = &ctx.current();
  auto documentation = Comment::parse(ctx);
  if (!documentation) {
    ctx.range_error(
        "TTX script is missing required documentation comment at start of "
        "file.",
        *token, ctx.current());

    documentation = "";
  }

  // Library Type
  token = &ctx.current();
  auto library = create_library(host, ctx, documentation.value());

  while (auto attribute = Attribute::parse(ctx)) {
  }

  while (token->valid()) {
    switch (token->klass) {
      case Classifier::Comment:
        documentation = Comment::parse(ctx);
        break;

      default:
        // Ignore for now...
        documentation = std::nullopt;
        break;
    }
    token = &ctx.advance();
  }

  // Add the compile unit to the program.
  // To enable incremental construction make sure to
  Types::Library* free_library = library.release();
  host.create_compile_unit(ctx.disk_path, free_library);

  // Return the pointer which is now owned by host.
  return free_library;
}
