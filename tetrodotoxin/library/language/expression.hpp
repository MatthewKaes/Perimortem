// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/reference/concept/layout.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/ffi/cpp/domain.hpp"
#include "ttx/reference/model/layouts/fluid.hpp"
#include "ttx/reference/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Language {

// Expression is the Abstract contract for one evaluatable source node.
// Expression identity remains distinct from Type identity so two values of the
// same Type remain distinct producers in the semantic DAG. Scalar Expressions
// produce one value. An owner such as Call may retain a complete empty or
// multiple result Layout while get_type() exposes a scalar Type only when
// exactly one result is available.
//
// Authored Expressions retain one lexical Anchor containing their complete
// Span and the independent Token a diagnostic should emphasize. Synthetic
// Expressions retain no Anchor because they have no authored location.
// This distinction remains independent from folding and lowering.
//
// get_type() returns the one scalar Type produced by the expression or Unknown
// when the source owner cannot establish exactly one. Concrete owners retain
// their real evaluation edges. Expression does not reconstruct those edges as
// a second generic input Layout. Library owns parsing, operator legality,
// executable bodies, and value fitting.
class Expression : public Ttx::Concept::Abstract, public Model::Pack {
 public:
  class Error {
   public:
    enum class Type : U8 {
      Unknown = U8(-1),
      InvalidOperationType = 0,
      InvalidInput,
      InvalidConstant,
      ResultTypeMismatch,
      ArithmeticOverflow,
      DivisionByZero,
    };

    constexpr Error(Type type, const Ttx::Concept::Abstract& subject)
        : type(type), subject(subject) {}
    static auto from_pack(Type type, const Model::Pack& subject) -> Error;

    constexpr auto get_type() const -> Type { return type; }
    constexpr auto get_subject() const -> const Ttx::Concept::Abstract& {
      return subject;
    }
    auto get_name() const -> Perimortem::Core::View::Bytes;

   private:
    Type type;
    const Ttx::Concept::Abstract& subject;
  };


  // Authored meaning is the only concept shared by every Expression. A
  // concrete computational owner may independently answer the ordinary
  // "fold" question; Expression does not define or cache that protocol.
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  // The result is the exact semantic object produced by this node. Ordinary
  // value Expressions produce themselves. Access nodes override this only
  // when evaluation selects an existing Type or Addressable identity. Keeping
  // result identity separate from get_type() lets Type valued expressions
  // remain available to later access without inventing a value output.
  virtual constexpr auto get_result() const
      -> const Ttx::Concept::Abstract& override {
    return *this;
  }

  auto get_identity() const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override {
    return *this;
  }

  // Assignment asks the completed expression for its writable value Type.
  // Ordinary access results delegate authority to their real Addressable.
  // Expressions such as Index may override the query when their semantics
  // deliberately provide a writable address without another identity.
  virtual auto get_write_type(const Model::Type& access_scope) const
      -> Perimortem::Core::Option<const Model::Type&>;

  virtual constexpr auto get_type() const
      -> const Ttx::Concept::Abstract& override = 0;

  // Library exposes an expression's scalar value relationship through the
  // host-neutral Domain operation. Empty and multi-value expressions keep
  // get_type() at Unknown, while a completed scalar forwards the exact
  // Library Type and its current Layout without teaching Abstract about this
  // language's type system.
  void domain(ttx_abstract self, ttx_domain_result result) const override;

  auto get_value_type(Count index) const
      -> const Ttx::Concept::Abstract& override;

  // Layout inspection is total. An ordinary value Expression exposes one
  // entry while a Type valued or incomplete Expression exposes an empty shape
  // and still resolves Unknown. Owners such as Call, Swizzle, and Slice
  // override this query when they produce complete empty or multiple value
  // flow without inventing an aggregate Type.
  auto get_layout() const -> const Ttx::Concept::Layout& override;

  // A linked Expression is a completed Pack. Multiple result owners override
  // this when their completion is not represented by one scalar Type edge.
  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto is_complete() const -> Bool override { return &resolve() == this; }

  // Expression finalization preserves this exact node. Graph questions remain
  // live and are never converted into finalization state.
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  // Write target lowering evaluates only the receiver and selector identities
  // needed to publish the destination. The write operation lowers its source
  // and then performs the actual mutation through its selected terminal
  // producer.

  // Linking enriches this exact source node after every declaration identity
  // is available. Constants already carry complete Types, while Identifier
  // and Operation owners attach their existing graph edges without replacing
  // the authored Expression. Lexical context owns name and shadowing order.
  // access scope carries only the host Type authority used by explicit member
  // and construction access. Keeping those inputs separate prevents hosting
  // from becoming an implicit receiver. An absent scope represents an unhosted
  // query and grants no private access.
  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  constexpr auto get_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return anchor;
  }

  // An empty inspection shape is not produced flow until resolve() proves this
  // exact Pack. Keeping the check here prevents direct fitting from admitting
  // a Type result through an empty target Layout.
  auto fits(const Ttx::Concept::Layout& target) const -> Bool override {
    return &resolve() == this && Model::Pack::fits(target);
  }

  // Ordinary expressions supply the complete Layout of their output Type.
  // Atomic Types retain their own exact identity as one terminal value while
  // structural Types expose their real shapes. Constant domains may extend
  // this rule when their value proves a contextual conversion safe.
  auto fits(const Ttx::Model::Domain& target) const -> Bool override {
    if (&resolve() != this) {
      return False;
    }

    auto source_type = get_type().select<Model::Type>();
    if (!source_type) {
      source_type = get_type().resolve().select<Model::Type>();
    }

    // Scalar Expressions compare their exact output Type, including
    // Constant owned contextual conversions. Multi value Packs have no scalar
    // Type and negotiate through their complete flow Layout instead.
    return source_type ? source_type->get_layout().fits(target.get_layout())
                       : Model::Pack::fits(target);
  }

  // Write operators use this one receiving Expression operation rather than
  // selecting Address, Index, Field, Local, or another concrete target. It
  // links the target and source in authored order, then lets the target decide
  // whether it accepts the complete source Pack. Plain value linking remains a
  // distinct operation so reference only Expressions can reject ordinary reads.
  virtual auto link_write(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      const Model::Type& access_scope,
      Model::Pack& source) -> Bool;

  auto link_write_restored(
      const Ttx::Concept::Abstract& lexical_context,
      const Model::Type& access_scope,
      Model::Pack& source) -> Bool;

 protected:
  // Concrete owners supply the builder because only their factory may use the
  // private constructor. The optional Anchor records whether source authored
  // the node while Arena begins its lifetime once at the final address.
  template <typename type, typename builder_type>
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Anchor anchor,
      builder_type&& builder) -> type& {
    static_assert(__is_base_of(Expression, type));
    Perimortem::Core::Option<Ttx::Lexical::Anchor> source(anchor);
    return domain.construct_from<type>([&builder, source]() {
      return static_cast<builder_type&&>(builder)(source);
    });
  }

  template <typename type, typename builder_type>
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      builder_type&& builder) -> type& {
    static_assert(__is_base_of(Expression, type));
    Perimortem::Core::Option<Ttx::Lexical::Anchor> source;
    return domain.construct_from<type>([&builder, source]() {
      return static_cast<builder_type&&>(builder)(source);
    });
  }

  constexpr explicit Expression(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : anchor(anchor), output_layout(*this, 1) {}

  Expression(const Expression&) = delete;
  Expression(Expression&&) = delete;
  auto operator=(const Expression&) -> Expression& = delete;
  auto operator=(Expression&&) -> Expression& = delete;

  // Ordinary writable Expressions link through their value path. A
  // reference only owner such as Index overrides this hook to establish its
  // destination relationship without admitting an ordinary read.
  virtual auto link_write_target(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      const Model::Type& access_scope) -> Bool;

  virtual auto link_write_target_restored(
      const Ttx::Concept::Abstract& lexical_context,
      const Model::Type& access_scope) -> Bool;

  // Complete Pack admission belongs to the receiving Expression. The default
  // delegates authority to the selected Addressable and its exact Type. Index
  // supplies scalar or ranged reference compatibility directly.
  virtual auto accepts_write(
      const Model::Pack& source,
      const Model::Type& access_scope) const -> Bool;

 private:
  Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor;
  Ttx::Model::Layouts::Ranged output_layout;
};

static_assert(__is_trivially_destructible(Expression::Error));

}  // namespace Tetrodotoxin::Library::Language
