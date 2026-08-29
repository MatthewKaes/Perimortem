// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/source.hpp"

#include <cstdio>

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "puffer/publisher.hpp"
#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"
#include "tetrodotoxin/shader/dialect.hpp"
#include "tetrodotoxin/terminal/abi/compiler.hpp"
#include "tetrodotoxin/terminal/graph_text.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/context.h"
#include "ttx/model/layouts/fluid.h"

using namespace Perimortem;
using namespace Tetrodotoxin;

class AbstractAppender {
 public:
  explicit AbstractAppender(
      Memory::Managed::Vector<const ttx_abstract*>& values)
      : callable{&operations}, operations{.call = append}, values(values) {}

  ttx_abstract_callable callable;

 private:
  static auto append(
      ttx_abstract_callable* callable,
      const ttx_abstract* abstract) -> void {
    auto& self = *reinterpret_cast<AbstractAppender*>(callable);
    self.values.insert(abstract);
  }

  ttx_abstract_callable_operations operations;
  Memory::Managed::Vector<const ttx_abstract*>& values;
};

static auto write_error(Core::View::Bytes message) -> void {
  fwrite(message.get_data(), 1, CppSize(message.get_size()), stderr);
  fwrite("\n", 1, 1, stderr);
}

static auto report_errors(const Ttx::Lexical::Errors& errors) -> void {
  Memory::Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    Core::View::Bytes message = errors.render_message(arena, index);
    write_error(message);
  }
}

static auto install(Environment::Toolchain& toolchain) -> Bool {
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto render = toolchain.install<Render::Dialect>("Pipeline"_view);
  return library && render &&
         toolchain.install<Package::Dialect>("Package"_view, *library) &&
         toolchain.install<App::Dialect>("App"_view) &&
         toolchain.install<Scene::Dialect>("Scene"_view, *library) &&
         toolchain.install<Shader::Dialect>("Shader"_view, *library, *render);
}

static auto is_package_source(Core::View::Bytes source, Core::View::Bytes path)
    -> Bool {
  Memory::Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena, source, path);
  auto tokens = tokenizer.get_tokens();
  for (Count index = 0; index + 2 < tokens.get_size(); index++) {
    if (tokens.get_data()[index].get_code() ==
            Ttx::Lexical::Code::Type::Dialect &&
        tokens.get_data()[index + 1].get_code() ==
            Ttx::Lexical::Code::Type::Define &&
        tokens.get_data()[index + 2].caculate_text(source) == "Package"_view) {
      return True;
    }
  }
  return False;
}

static auto product_path(
    Memory::Allocator::Arena& arena,
    const Package::Language::Monograph& package,
    Core::View::Bytes file) -> Core::View::Bytes {
  Memory::Managed::Bytes path(arena);
  Serialization::Stream::Textual<Memory::Managed::Bytes> output(path);
  output << package.get_name() << "/"_view << package.get_version().get_major()
         << "."_view << package.get_version().get_minor() << "/"_view << file;
  return path.get_view();
}

static auto append_cxx_products(
    Memory::Allocator::Arena& arena,
    const Language::Monograph& monograph,
    Ttx::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text,
    Memory::Managed::Vector<const ttx_abstract*>& products) -> Bool {
  auto package = monograph.select<Package::Language::Monograph>();
  BAIL_IF(!package);

  const Terminal::Abi::Unit unit(
      package->get_name(), "api"_view, "api"_view, {}, {}, {}, {}, "api.h"_view,
      "api.hpp"_view);
  Terminal::Abi::Compiler compiler;
  auto generated = compiler.compile_graph(
      arena, package->get_library(), *package, unit, errors, source_path,
      source_text);
  BAIL_IF(!generated);

  products.insert(
      Language::Product::create(
          arena, product_path(arena, *package, "api.h"_view),
          generated->get_c_header())
          .get_abi());
  products.insert(
      Language::Product::create(
          arena, product_path(arena, *package, "api.hpp"_view),
          generated->get_cpp_header())
          .get_abi());
  products.insert(
      Language::Product::create(
          arena, product_path(arena, *package, "api.cpp"_view),
          generated->get_cpp_source())
          .get_abi());
  return True;
}

auto Puffer::Source::run() const -> S32 {
  auto contents = System::File::read(source);
  if (!contents) {
    write_error("puffer: source could not be read"_view);
    return 1;
  }

  Environment::Toolchain toolchain;
  if (!install(toolchain)) {
    write_error("puffer: built-in Dialect installation failed"_view);
    return 1;
  }

  Memory::Allocator::Arena products;
  Ttx::Lexical::Errors errors;
  Environment::Workspace workspace(toolchain, {}, &repository);
  const Language::Monograph* monograph = nullptr;
  Bool completed = False;
  if (is_package_source(contents->get_view(), source)) {
    System::Path source_path(source);
    Core::View::Bytes root = source_path.get_directory();
    if (root.is_empty()) {
      root = "."_view;
    }
    Core::View::Bytes route = source_path.get_file();
    auto imported = workspace.import_package(errors, root, source, route);
    if (imported) {
      monograph = &*imported;
      completed = True;
    } else {
      auto retained = workspace.get_monograph(root, route);
      if (retained) {
        monograph = &*retained;
      }
    }
  } else {
    auto interpreted = workspace.interpret_source(
        errors, source, source, contents->get_view());
    if (interpreted) {
      monograph = &*interpreted;
      completed = True;
    } else {
      auto retained = workspace.get_monograph(source);
      if (retained) {
        monograph = &*retained;
      }
    }
  }

  if (monograph == nullptr) {
    report_errors(errors);
    return 1;
  }
  if (dump_graph) {
    const Language::Product& product = Terminal::GraphText::write(
        products, source, monograph->get_language().get_abi(),
        monograph->get_root().get_abi(), workspace.get_abi());
    fwrite(
        product.get_value().get_data(), 1,
        CppSize(product.get_value().get_size()), stdout);
  }
  if (!completed) {
    report_errors(errors);
    return 1;
  }
  auto dialect = monograph->get_language().select<Language::Dialect>();
  const ttx_pack* produced =
      dialect ? dialect->produce(products, workspace, *monograph) : nullptr;
  Memory::Managed::Vector<const ttx_abstract*> publications(products);
  if (generate_cxx && !append_cxx_products(
                          products, *monograph, errors, source,
                          contents->get_view(), publications)) {
    report_errors(errors);
    write_error("puffer: selected graph does not support C++ generation"_view);
    return 1;
  }
  if (produced == nullptr) {
    write_error("puffer: selected Dialect could not publish its product"_view);
    return 1;
  }
  AbstractAppender appender(publications);
  ttx_layout_visit(ttx_pack_layout(produced), &appender.callable);
  ttx_fluid_layout layout;
  ttx_model_pack context_packs[1];
  ttx_fluid_layout context_layouts[1];
  ttx_named_layout context_named_layouts[1];
  Memory::Managed::Vector<const ttx_abstract*> context_entries(products);
  for (Count index = 0; index < publications.get_size(); index++) {
    context_entries.insert(nullptr);
  }
  ttx_model_context context;
  ttx_fluid_layout_initialize(
      &layout, publications.get_view().get_data(), publications.get_size());
  ttx_model_context_initialize(
      &context,
      ttx_model_context_storage{
        .packs = context_packs,
        .layouts = context_layouts,
        .named_layouts = context_named_layouts,
        .entries = context_entries.is_empty() ? nullptr : &context_entries[0],
        .names = nullptr,
        .name_bytes = nullptr,
        .pack_capacity = 1,
        .entry_capacity = publications.get_size(),
        .name_capacity = 0,
        .name_byte_capacity = 0,
      });
  const ttx_pack* publication =
      ttx_context_pack(&context.context, &layout.layout);
  if (publication == nullptr ||
      !Publisher(terminal_root).publish(publication)) {
    write_error("puffer: product publication failed"_view);
    return 1;
  }
  return 0;
}
