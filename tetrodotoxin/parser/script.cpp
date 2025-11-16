// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/script.hpp"
#include "parser/context.hpp"
#include "parser/visitor/attribute.hpp"
#include "parser/visitor/comment.hpp"

#include "types/library.hpp"

#include <filesystem>

using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Parser;
using namespace Tetrodotoxin::Types;

auto detect_package_type(Context& ctx) -> void {
  auto token = &ctx.current();
  if (!ctx.check_klass(Classifier::Package))
    return;

  token = &ctx.advance();

  if (token->klass == Classifier::Entity) {
    ctx.library.set_entity(true);
  } else if (token->klass != Classifier::Library) {
    ctx.token_error(std::format(
        "Unknown package type `{}`, defaulting to `library`.", token->data));
  };
  ctx.advance();

  // If we correctly have an end statement then consume it, otherwise report a
  // non-fatal token class error.
  if (ctx.check_klass(Classifier::EndStatement)) {
    // Eat end statement.
    token = &ctx.advance();
  }
}

// Registers a name, note for error reporting the ctx state is assumed to be
// right after parsing the provided abstract.
auto register_name(Context& ctx,
                   const Token& start_token,
                   const Perimortem::Memory::ManagedString& name,
                   const Abstract* abstract) {
  if (!ctx.library.create_name(name, abstract)) {
    ctx.range_error(
        std::format("Name '{}' has already been declared.", name.get_view()),
        start_token, *(&ctx.current() - 1));
  }
}

auto Script::parse(Types::Program& host,
                   Errors& errors,
                   const std::filesystem::path& source_map,
                   const std::string_view& source) -> Types::Library& {
  auto& library = host.create_compile_unit(source_map);
  library.load(source, optimize);
  Context ctx(library, source_map, errors);

  if (library.tokenizer.empty()) {
    ctx.generic_error("Empty source file provided for parsing.");
    return library;
  }

  // Parse the required TTX documentation comment.
  auto token = &ctx.current();
  auto documentation = Visitor::parse_comment(ctx);
  if (documentation.empty()) {
    ctx.range_error(
        "TTX script is missing required documentation comment at start of "
        "file.",
        *token, ctx.current());
  }

  // Detect the package type.
  detect_package_type(ctx);
  library.set_doc(documentation);

  // Library Type
  token = &ctx.current();
  const Token* comment_start = nullptr;

  while (token->valid()) {
    switch (token->klass) {
      // Comment in Tetrodotoxin are not allowed to be free and must be attached
      // to a context. Free comments are an error and will be dropped from the
      // context.
      case Classifier::Comment:
        comment_start = &ctx.current();
        documentation = Visitor::parse_comment(ctx);
        token = &ctx.advance();
        continue;

      case Classifier::Attribute: {
        auto attribute = Visitor::parse_attribute(ctx);
        attribute->doc.take(documentation);
        register_name(ctx, *token, attribute->name, attribute);

        // Attribute for setting the package name.
        if (attribute->name == "@Name") {
          library.set_name(attribute->value);
        }
        break;
      }

      default:
        // Ignore for now...
        documentation.clear();
        break;
    }
    token = &ctx.advance();

    // Free floating documentation.
    if (!documentation.empty()) {
      ctx.range_error(
          "TTX does not support top level floating comments. Comment blocks "
          "must be attached to a top level definition.",
          *comment_start, ctx.current());
      documentation.clear();
    }
  }

  return library;
}
