// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

namespace Tetrodotoxin::Resolution {

class Source;

class ImportRequest {
 public:
  ImportRequest() = default;
  ImportRequest(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes dialect_name,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes package_path)
      : local_name(local_name),
        dialect_name(dialect_name),
        source_path(source_path),
        package_path(package_path) {}

  static auto file(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes dialect_name,
      Perimortem::Core::View::Bytes source_path) -> ImportRequest {
    return {local_name, dialect_name, source_path, {}};
  }

  static auto package(
      Perimortem::Core::View::Bytes local_name,
      Perimortem::Core::View::Bytes dialect_name,
      Perimortem::Core::View::Bytes package_path) -> ImportRequest {
    return {local_name, dialect_name, {}, package_path};
  }

  auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }
  auto get_dialect_name() const -> Perimortem::Core::View::Bytes {
    return dialect_name;
  }
  auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  auto get_package_path() const -> Perimortem::Core::View::Bytes {
    return package_path;
  }
  auto is_file() const -> Bool { return !source_path.is_empty(); }
  auto is_package() const -> Bool { return !package_path.is_empty(); }

 private:
  Perimortem::Core::View::Bytes local_name;
  Perimortem::Core::View::Bytes dialect_name;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes package_path;
};

class ImportBinding {
 public:
  ImportBinding() = default;
  ImportBinding(const ImportRequest& request, Source* source)
      : request(request), source(source) {}

  auto get_request() const -> const ImportRequest& { return request; }
  auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return request.get_local_name();
  }
  auto get_dialect_name() const -> Perimortem::Core::View::Bytes {
    return request.get_dialect_name();
  }
  auto get_source() const -> Source* { return source; }
  auto is_resolved() const -> Bool { return source != nullptr; }

 private:
  ImportRequest request;
  Source* source = nullptr;
};

class Source {
 public:
  Source() = default;
  explicit Source(Perimortem::Core::View::Bytes path) : path(path) {}

  auto get_path() const -> Perimortem::Core::View::Bytes { return path; }
  auto get_dialect_name() const -> Perimortem::Core::View::Bytes {
    return dialect_name;
  }
  auto get_imports() const
      -> Perimortem::Core::View::Vector<ImportRequest> {
    return imports.get_view();
  }
  auto get_bindings() const
      -> Perimortem::Core::View::Vector<ImportBinding> {
    return bindings.get_view();
  }

  auto set_dialect_name(Perimortem::Core::View::Bytes name) -> void {
    dialect_name = name;
  }
  auto add_import(const ImportRequest& import) -> void {
    imports.insert(import);
  }
  auto bind_import(const ImportRequest& import, Source* source) -> void {
    bindings.insert({import, source});
  }

 private:
  Perimortem::Core::View::Bytes path;
  Perimortem::Core::View::Bytes dialect_name;
  Perimortem::Memory::Dynamic::Vector<ImportRequest> imports;
  Perimortem::Memory::Dynamic::Vector<ImportBinding> bindings;
};

class Resolver {
 public:
  Resolver() = default;
  ~Resolver();

  auto load(Perimortem::Core::View::Bytes source_path) -> Source*;
  auto resolve(Perimortem::Core::View::Bytes source_path) -> Bool;
  auto find(Perimortem::Core::View::Bytes source_path) const -> Source*;
  auto get_sources() const -> Perimortem::Core::View::Vector<Source*> {
    return sources.get_view();
  }

 private:
  Perimortem::Memory::Dynamic::Vector<Source*> sources;
};

}  // namespace Tetrodotoxin::Resolution
