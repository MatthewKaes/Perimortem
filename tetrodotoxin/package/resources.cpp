// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/resources.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/path.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static auto construct_resource(Allocator::Arena& domain, View::Bytes value)
    -> Tetrodotoxin::Language::Resource& {
  class RetainedResource : public Tetrodotoxin::Language::Resource {
   public:
    constexpr RetainedResource(View::Bytes value) : value(value) {}

    constexpr auto get_value() const -> View::Bytes override { return value; }

   private:
    View::Bytes value;
  };

  return domain.construct<RetainedResource>(value);
}

static auto construct_error(
    Allocator::Arena& domain,
    const Package::Storage::Failure& failure)
    -> Tetrodotoxin::Language::Error& {
  class RetainedError : public Tetrodotoxin::Language::Error {
   public:
    constexpr RetainedError(const Package::Storage::Failure& failure)
        : failure(failure) {}

    auto describe(Errors::Report& report) const -> void override {
      auto path = failure.get_path();
      switch (failure.get_error()) {
      case Package::Storage::Failure::Error::InvalidRoute:
        path.visit(
            [&]() {
              report << "Package resource route is empty or lexically "
                        "invalid."_view;
            },
            [&](const Path& selected) {
              report << "Package resource route "_view << selected.get_view()
                     << " is not a confined logical child."_view;
            });
        return;
      case Package::Storage::Failure::Error::Unreadable:
        path.visit(
            [&]() {
              report << "Package resource could not be read through its "
                        "opened root."_view;
            },
            [&](const Path& selected) {
              report << "Package resource "_view << selected.get_view()
                     << " could not be read through its opened root."_view;
            });
        return;
      default:
        report << "Package resource acquisition failed for an unknown "
                  "reason."_view;
        return;
      }
    }

   private:
    Package::Storage::Failure failure;
  };

  return domain.construct<RetainedError>(failure);
}

auto Package::Resources::connect(Storage& selected) -> Bool {
  if (stage != Stage::Pending) {
    return False;
  }

  storage = &selected;
  stage = Stage::Connected;
  return True;
}

auto Package::Resources::seal() -> void {
  storage = nullptr;
  stage = Stage::Sealed;
}

auto Package::Resources::resolve(View::Bytes logical_route) -> const Abstract& {
  // The bounded Path is only a cache probe. Storage still owns acceptance and
  // supplies the durable key for the first request, so Resources never grows a
  // second confinement policy.
  Path normalized(logical_route);
  Dynamic::Bytes complete_route;
  View::Bytes request_key = normalized.get_view();
  if (request_key.is_empty()) {
    // A complete resource route has fixed delimiters. Rebuilding them here
    // keeps invalid spellings exact while Monograph delegates only the interior
    // logical route.
    complete_route.concat("$["_view);
    complete_route.concat(logical_route);
    complete_route.append(']');
    request_key = complete_route;
  }

  auto cached_resource = resource_cache.find(request_key);
  if (cached_resource) {
    return cached_resource->value;
  }

  auto cached_error = error_cache.find(request_key);
  if (cached_error) {
    return cached_error->value;
  }

  // Sealing removes the only physical capability. Existing identities were
  // selected above, while a new request stays Invalid instead of reopening or
  // retaining Storage through the semantic Package graph.
  if (stage != Stage::Connected) {
    return Invalid::get_invalid();
  }

  auto read = storage->read(logical_route);
  return read.visit(
      [&](Package::Content& content) -> const Abstract& {
        View::Bytes retained_key = domain.proxy(content.get_diagnostic_path());
        Tetrodotoxin::Language::Resource& retained =
            construct_resource(domain, domain.proxy(content.get_contents()));
        resource_cache.launder(retained_key, retained);
        return retained;
      },
      [&](const Package::Storage::Failure& failure) -> const Abstract& {
        // Storage owns the typed category and normalized Path. Copy only that
        // Path into the identity and Arena key, while invalid input without a
        // Path keeps its complete authored spelling distinct.
        View::Bytes retained_key;
        failure.get_path().visit(
            [&]() { retained_key = domain.proxy(request_key); },
            [&](const Path& path) {
              retained_key = domain.proxy(path.get_view());
            });

        Tetrodotoxin::Language::Error& retained =
            construct_error(domain, failure);
        error_cache.launder(retained_key, retained);
        return retained;
      });
}
