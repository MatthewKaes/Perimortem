// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/dialect/shader.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/syntax/attribute/common.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Syntax;

namespace Tetrodotoxin::Syntax {

class ShaderStageAttribute {
 public:
  static constexpr auto name = "stage"_view;

  static auto validate_member(
      Context& ctx,
      const Attribute::Target& target,
      const Ast::Attribute& attr) -> void {
    Attribute::require_named_fields(
        ctx, attr,
        "@stage uses named fields, e.g. @stage(.kind = \"vertex\")"_view);
    Attribute::require_field(
        ctx, attr, Attribute::FieldKind::Kind,
        "@stage requires .kind, e.g. @stage(.kind = \"pixel\")"_view);

    if (!target.type || target.type->get_kind() != DeclarationKind::Shader) {
      ctx.error(
          "Invalid @stage target"_view,
          "@stage is valid only on Shader definitions in Shader packages"_view);
    }
  }
};

class ShaderPushConstantAttribute {
 public:
  static constexpr auto name = "push_constant"_view;

  static auto validate_member(
      Context& ctx,
      const Attribute::Target& target,
      const Ast::Attribute& attr) -> void {
    if (!attr.get_fields().is_empty()) {
      ctx.error(
          "Unexpected @push_constant arguments"_view,
          "@push_constant is a marker attribute and does not take fields"_view);
    }

    if (!target.type || target.type->get_kind() != DeclarationKind::Foreign) {
      ctx.error(
          "Invalid @push_constant target"_view,
          "@push_constant is valid only on Foreign ABI blocks in Shader "
          "packages"_view);
    }

    if (target.type && target.type->get_definition().get_sigil() !=
                           Class::Type::ConstDynamic) {
      ctx.error(
          "Expected exposed push constant ABI"_view,
          "Host-visible push constants should be declared as @expose Foreign "
          "blocks"_view);
    }
  }
};

class ShaderBindingAttribute {
 public:
  static constexpr auto name = "binding"_view;

  static auto validate_member(
      Context& ctx,
      const Attribute::Target& target,
      const Ast::Attribute& attr) -> void {
    Attribute::require_named_fields(
        ctx, attr,
        "@binding uses named fields, e.g. @binding(.set = 0, .slot = 0)"_view);
    Attribute::require_field(
        ctx, attr, Attribute::FieldKind::Set,
        "@binding requires .set, e.g. @binding(.set = 0, .slot = 0)"_view);
    Attribute::require_field(
        ctx, attr, Attribute::FieldKind::Slot,
        "@binding requires .slot, e.g. @binding(.set = 0, .slot = 0)"_view);

    if (target.scope_type != Class::Type::Shader ||
        !Attribute::is_value_member(target.member)) {
      ctx.error(
          "Invalid @binding target"_view,
          "@binding is valid only on resource members inside Shader scopes"_view);
    }
  }
};

class ShaderBuiltinAttribute {
 public:
  static constexpr auto name = "builtin"_view;

  static auto validate_member(
      Context& ctx,
      const Attribute::Target& target,
      const Ast::Attribute& attr) -> void {
    validate_fields(ctx, attr);

    if (target.scope_type != Class::Type::Shader ||
        !Attribute::is_value_member(target.member)) {
      ctx.error(
          "Invalid @builtin target"_view,
          "@builtin on members is valid only for Shader builtin inputs"_view);
    }
  }

  static auto validate_layout(Context& ctx, const Ast::Attribute& attr)
      -> void {
    validate_fields(ctx, attr);
  }

 private:
  static auto validate_fields(Context& ctx, const Ast::Attribute& attr)
      -> void {
    Attribute::require_named_fields(
        ctx, attr,
        "@builtin uses named fields, e.g. @builtin(.name = \"position\")"_view);

    if (!Attribute::has_field(attr.get_fields(), Attribute::FieldKind::Name) &&
        !Attribute::has_field(attr.get_fields(), Attribute::FieldKind::Slot)) {
      ctx.error(
          "Expected builtin field"_view,
          "@builtin requires either .name or .slot"_view);
    }
  }
};

}  // namespace Tetrodotoxin::Syntax

using AttributeMapping = Pair<View::Bytes, Dialect::AttributeHandler>;
static constexpr Static::Vector<AttributeMapping, 4> attributes = {{
  {
    ShaderStageAttribute::name,
    {
      ShaderStageAttribute::validate_member,
      Attribute::reject_layout_attribute,
      True,
    },
  },
  {
    ShaderPushConstantAttribute::name,
    {
      ShaderPushConstantAttribute::validate_member,
      Attribute::reject_layout_attribute,
      True,
    },
  },
  {
    ShaderBindingAttribute::name,
    {
      ShaderBindingAttribute::validate_member,
      Attribute::reject_layout_attribute,
      True,
    },
  },
  {
    ShaderBuiltinAttribute::name,
    {
      ShaderBuiltinAttribute::validate_member,
      ShaderBuiltinAttribute::validate_layout,
      True,
    },
  },
}};

using AttributeTable = Table<Dialect::AttributeHandler, attributes>;

auto Dialect::Shader::get_kind() const -> Class {
  return Class::Type::Shader;
}

auto Dialect::Shader::find_attribute(View::Bytes name) const
    -> AttributeHandler {
  return AttributeTable::find_or_default(name, AttributeHandler{});
}
