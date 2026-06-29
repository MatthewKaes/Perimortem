// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// A single import declaration. Two source forms are supported:
//   import Name : Kind = ( .source = "path/to/file.ttx" ) ;
//   import Name : Kind = Package.Sub.Name ;
class Import {
 public:
  static auto parse(Context& ctx) -> Import;

  auto get_local_name() const -> Perimortem::Core::View::Bytes {
    return local_name;
  }

  auto get_class() const -> Lexical::Class { return klass; }
  auto get_source_class() const -> Lexical::Class { return source_klass; }
  auto is_file_source() const -> Bool {
    return source_klass == Lexical::Class::Type::PackedData;
  }
  auto is_package_source() const -> Bool {
    return source_klass == Lexical::Class::Type::Type;
  }

  // The file path string token text. Valid only when form == Form::File.
  auto get_file_path() const -> Perimortem::Core::View::Bytes {
    return file_path;
  }

  // The dot-separated type segments, e.g. ["TTX", "Log"]. Valid only when
  // form == Form::Package.
  auto get_package_path() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return package_alias.get_name_path();
  }
  auto get_package_alias() const -> const Type::Ref& { return package_alias; }

  auto is_valid() const -> Bool {
    return klass.get_type() != Lexical::Class::Type::EndOfStream;
  }

 private:
  Perimortem::Core::View::Bytes local_name;
  Lexical::Class klass = Lexical::Class::Type::EndOfStream;
  Lexical::Class source_klass = Lexical::Class::Type::EndOfStream;
  Perimortem::Core::View::Bytes file_path;
  Type::Ref package_alias;
};

}  // namespace Tetrodotoxin::Syntax::Ast
