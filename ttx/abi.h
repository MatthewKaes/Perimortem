// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_ABI_H
#define TTX_ABI_H

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
#define TTX_EXTERN_C extern "C"
#else
#define TTX_EXTERN_C
#endif

#if defined(_WIN32)
#define TTX_CALL __cdecl
#define TTX_EXPORT __declspec(dllexport)
#if defined(TTX_ABI_IMPLEMENTATION)
#define TTX_API TTX_EXPORT
#else
#define TTX_API __declspec(dllimport)
#endif
#else
#define TTX_CALL
#define TTX_EXPORT __attribute__((visibility("default")))
#define TTX_API TTX_EXPORT
#endif

#define TTX_ABI_MAJOR 1u
#define TTX_ABI_MINOR 0u

// Canonical ABI names do not repeat their version in every identifier. Each
// immutable operations table begins with this header instead. An importer
// checks the major version and the required prefix size before using a table,
// which leaves later owners free to append operations without changing the
// meaning of an established prefix.
typedef struct {
  uint32_t size;
  uint16_t abi_major;
  uint16_t abi_minor;
} ttx_abi_header;

typedef struct {
  const uint8_t* data;
  uint64_t size;
} ttx_borrowed_bytes;

// Most support handles pair an immutable operation table with private dispatch
// state. They carry no semantic identity, so that pair is only a call boundary
// for the owner which produced the support value.
#define TTX_DECLARE_HANDLE(name)          \
  typedef struct name##_ops name##_ops;   \
  typedef struct name##_self name##_self; \
  typedef struct {                        \
    const name##_ops* operations;         \
    name##_self* self;                    \
  } name

// An Abstract is different: the capability itself is the semantic identity.
// Passing its address keeps the operation table and the subject it serves
// inseparable. A C, C++, Rust, remote, or synthesized implementation may place
// any private state behind this prefix, but callers receive only this pointer
// and can neither replace its table nor recover a language object from it.
typedef struct ttx_abstract_ops ttx_abstract_ops;
typedef struct ttx_abstract_capability {
  const ttx_abstract_ops* operations;
} ttx_abstract_capability;
typedef const ttx_abstract_capability* ttx_abstract;

TTX_DECLARE_HANDLE(ttx_documentation);

// An Interface witness is also one capability. It is synthesized by the
// candidate answering a requirement and remains borrowed only for the sink
// callback which receives it. Keeping its dispatch attached to that witness
// prevents a caller from recombining one relation with another implementation.
typedef struct ttx_interface_ops ttx_interface_ops;
typedef struct ttx_interface_capability {
  const ttx_interface_ops* operations;
} ttx_interface_capability;
typedef const ttx_interface_capability* ttx_interface;

#if defined(__cplusplus)
static_assert(sizeof(ttx_abstract) == sizeof(void*));
static_assert(sizeof(ttx_interface) == sizeof(void*));
#else
_Static_assert(
    sizeof(ttx_abstract) == sizeof(void*),
    "Abstract is one pointer");
_Static_assert(
    sizeof(ttx_interface) == sizeof(void*),
    "Interface is one pointer");
#endif

TTX_DECLARE_HANDLE(ttx_layout);
TTX_DECLARE_HANDLE(ttx_pack);
TTX_DECLARE_HANDLE(ttx_context);
TTX_DECLARE_HANDLE(ttx_enumerable);
TTX_DECLARE_HANDLE(ttx_named);
TTX_DECLARE_HANDLE(ttx_layout_snapshot);
TTX_DECLARE_HANDLE(ttx_callable);
TTX_DECLARE_HANDLE(ttx_fluid);
TTX_DECLARE_HANDLE(ttx_route);
TTX_DECLARE_HANDLE(ttx_value_layout);
TTX_DECLARE_HANDLE(ttx_composite_layout);
TTX_DECLARE_HANDLE(ttx_finite_extent);
TTX_DECLARE_HANDLE(ttx_ranged_layout);
TTX_DECLARE_HANDLE(ttx_reindexed_layout);
TTX_DECLARE_HANDLE(ttx_bytes);
TTX_DECLARE_HANDLE(ttx_addressable_policy);

TTX_DECLARE_HANDLE(ttx_bytes_sink);
TTX_DECLARE_HANDLE(ttx_abstract_sink);
TTX_DECLARE_HANDLE(ttx_concept_sink);
TTX_DECLARE_HANDLE(ttx_interface_sink);
TTX_DECLARE_HANDLE(ttx_domain_result);
TTX_DECLARE_HANDLE(ttx_pack_result);
TTX_DECLARE_HANDLE(ttx_enumerable_result);
TTX_DECLARE_HANDLE(ttx_named_result);
TTX_DECLARE_HANDLE(ttx_layout_snapshot_result);
TTX_DECLARE_HANDLE(ttx_callable_result);
TTX_DECLARE_HANDLE(ttx_fluid_result);
TTX_DECLARE_HANDLE(ttx_route_result);
TTX_DECLARE_HANDLE(ttx_value_layout_result);
TTX_DECLARE_HANDLE(ttx_composite_layout_result);
TTX_DECLARE_HANDLE(ttx_finite_extent_result);
TTX_DECLARE_HANDLE(ttx_ranged_layout_result);
TTX_DECLARE_HANDLE(ttx_reindexed_layout_result);
TTX_DECLARE_HANDLE(ttx_bytes_result);
TTX_DECLARE_HANDLE(ttx_named_selection_result);
TTX_DECLARE_HANDLE(ttx_layout_entry_sink);
TTX_DECLARE_HANDLE(ttx_named_route_sink);
TTX_DECLARE_HANDLE(ttx_reindex_sink);
TTX_DECLARE_HANDLE(ttx_addressable_interface_result);

// Abstract handles borrow one semantic identity from its owning graph.
// Documentation and Layout handles borrow immutable projections. Interface
// handles are narrower: they remain valid only inside the sink callback that
// receives them. A Layout snapshot transfers owned support with an explicit
// release operation. Context owns those snapshots and its Packs until release.
//
// Every sink is call-scoped. A single-result operation selects one result arm
// exactly once before returning. A visitor may emit any finite number of items
// and then completes exactly once. Providers retain neither a sink nor a
// borrowed byte view after the operation returns.
// Typed category views may also be synthesized inside their result callback.
// A consumer needing them afterward copies their public support while that
// callback is active. Copying a Callable uses each Layout's snapshot operation.
// The Abstract candidates inside those copies remain borrowed graph identities.

typedef enum {
  TTX_INTERFACE_UNKNOWN = 0,
  TTX_INTERFACE_REJECTED = 1,
  TTX_INTERFACE_SATISFIED = 2,
  TTX_INTERFACE_EQUIVALENT = 3
} ttx_interface_relation;

// Unknown keeps this ordered requirement/candidate relationship open.
// Rejected completes it negatively, Satisfied proves the requested direction,
// and Equivalent records the stronger relation established by that exact
// requirement. None of these outcomes changes either participant's identity.

typedef enum {
  TTX_PACK_SUPPORT_INVALID_LAYOUT = 1,
  TTX_PACK_SUPPORT_EXHAUSTED = 2
} ttx_pack_support_failure;

struct ttx_bytes_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* bytes)(ttx_bytes_sink, ttx_borrowed_bytes);
  void(TTX_CALL* completed)(ttx_bytes_sink);
};

struct ttx_abstract_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* answer)(ttx_abstract_sink, ttx_abstract);
};

struct ttx_concept_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* item)(
      ttx_concept_sink,
      ttx_borrowed_bytes route,
      ttx_abstract answer);
  void(TTX_CALL* completed)(ttx_concept_sink);
};

struct ttx_interface_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* answer)(ttx_interface_sink, ttx_interface);
};

struct ttx_domain_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(ttx_domain_result);
  void(TTX_CALL* none)(ttx_domain_result);
  void(TTX_CALL* resolved)(
      ttx_domain_result,
      ttx_abstract domain,
      ttx_layout layout);
};

// A resolved Domain result carries the exact Domain Abstract and its immutable
// Layout for this observation. Returning both through the typed callback lets a
// provider synthesize the Layout without exposing a native Domain object.

struct ttx_pack_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(ttx_pack_result);
  void(TTX_CALL* none)(ttx_pack_result);
  void(TTX_CALL* packed)(ttx_pack_result, ttx_pack);
  void(TTX_CALL* support_failed)(ttx_pack_result, ttx_pack_support_failure);
};

// unknown and none are semantic fitting answers. support_failed reports that
// the Context could not retain otherwise valid support data; a host allocation
// or transport failure therefore cannot masquerade as a semantic answer.

struct ttx_enumerable_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_enumerable_result);
  void(TTX_CALL* satisfied)(ttx_enumerable_result, ttx_enumerable);
};

struct ttx_named_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_named_result);
  void(TTX_CALL* satisfied)(ttx_named_result, ttx_named);
};

struct ttx_layout_snapshot_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* retained)(ttx_layout_snapshot_result, ttx_layout_snapshot);
  void(TTX_CALL* support_failed)(
      ttx_layout_snapshot_result,
      ttx_pack_support_failure);
};

struct ttx_callable_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(ttx_callable_result);
  void(TTX_CALL* none)(ttx_callable_result);
  void(TTX_CALL* resolved)(ttx_callable_result, ttx_callable);
  void(TTX_CALL* support_failed)(ttx_callable_result, ttx_pack_support_failure);
};

// The resolved Callable view is borrowed during this callback. Its parameter
// and result Layouts can also be synthesized there. A caller retaining the
// observation requests Layout snapshots before returning, keeping support
// allocation failure separate from the candidate's semantic answer.

struct ttx_fluid_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_fluid_result);
  void(TTX_CALL* satisfied)(ttx_fluid_result, ttx_fluid);
};

struct ttx_route_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(ttx_route_result);
  void(TTX_CALL* none)(ttx_route_result);
  void(TTX_CALL* resolved)(ttx_route_result, ttx_route);
};

struct ttx_value_layout_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_value_layout_result);
  void(TTX_CALL* satisfied)(ttx_value_layout_result, ttx_value_layout);
};

struct ttx_composite_layout_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_composite_layout_result);
  void(TTX_CALL* satisfied)(ttx_composite_layout_result, ttx_composite_layout);
};

struct ttx_finite_extent_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(ttx_finite_extent_result);
  void(TTX_CALL* none)(ttx_finite_extent_result);
  void(TTX_CALL* resolved)(ttx_finite_extent_result, ttx_finite_extent);
};

struct ttx_ranged_layout_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_ranged_layout_result);
  void(TTX_CALL* satisfied)(ttx_ranged_layout_result, ttx_ranged_layout);
};

struct ttx_reindexed_layout_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* rejected)(ttx_reindexed_layout_result);
  void(TTX_CALL* satisfied)(ttx_reindexed_layout_result, ttx_reindexed_layout);
};

struct ttx_bytes_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(ttx_bytes_result);
  void(TTX_CALL* none)(ttx_bytes_result);
  void(TTX_CALL* resolved)(ttx_bytes_result, ttx_bytes);
};

struct ttx_addressable_interface_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* pass)(ttx_addressable_interface_result);
  void(TTX_CALL* answer)(
      ttx_addressable_interface_result,
      ttx_interface_relation relation);
};

struct ttx_named_selection_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(ttx_named_selection_result);
  void(TTX_CALL* none)(ttx_named_selection_result);
  void(TTX_CALL* selected)(
      ttx_named_selection_result,
      ttx_borrowed_bytes structural_path);
};

struct ttx_layout_entry_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* entry)(
      ttx_layout_entry_sink,
      ttx_borrowed_bytes structural_path,
      ttx_abstract producer);
  void(TTX_CALL* completed)(ttx_layout_entry_sink);
};

struct ttx_named_route_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* unknown)(
      ttx_named_route_sink,
      ttx_borrowed_bytes structural_path);
  void(
      TTX_CALL* none)(ttx_named_route_sink, ttx_borrowed_bytes structural_path);
  void(TTX_CALL* route)(
      ttx_named_route_sink,
      ttx_borrowed_bytes structural_path,
      ttx_borrowed_bytes complete_route);
  void(TTX_CALL* completed)(ttx_named_route_sink);
};

struct ttx_reindex_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* mapping)(
      ttx_reindex_sink,
      ttx_borrowed_bytes output_path,
      ttx_borrowed_bytes source_path);
  void(TTX_CALL* completed)(ttx_reindex_sink);
};

struct ttx_documentation_ops {
  ttx_abi_header header;
  uint64_t(TTX_CALL* size)(ttx_documentation);
  void(TTX_CALL* visit_bytes)(ttx_documentation, ttx_bytes_sink);
};

struct ttx_interface_ops {
  ttx_abi_header header;
  ttx_abstract(TTX_CALL* requirement)(ttx_interface);
  ttx_abstract(TTX_CALL* candidate)(ttx_interface);
  ttx_interface_relation(TTX_CALL* negotiate)(ttx_interface);

  // A positive relationship invokes one exact requirement-owned operation.
  // Arguments and results remain Packs; concrete representations leave through
  // independently supplied Terminals rather than widening this base contract.
  void(TTX_CALL* invoke)(
      ttx_interface,
      ttx_abstract operation,
      ttx_pack input,
      ttx_context,
      ttx_pack_result);
};

#define TTX_INTERFACE_RELATION_PREFIX_SIZE \
  ((uint32_t)offsetof(ttx_interface_ops, invoke))

struct ttx_abstract_ops {
  ttx_abi_header header;
  ttx_borrowed_bytes(TTX_CALL* name)(ttx_abstract);
  ttx_documentation(TTX_CALL* documentation)(ttx_abstract);
  void(TTX_CALL* resolve)(ttx_abstract, ttx_abstract_sink);
  void(TTX_CALL* resolve_concept)(
      ttx_abstract,
      ttx_borrowed_bytes route,
      ttx_abstract_sink);
  void(TTX_CALL* visit_concepts)(ttx_abstract, ttx_concept_sink);
  void(TTX_CALL* interface)(
      ttx_abstract,
      ttx_abstract requirement,
      ttx_interface_sink);

  // Chapters 1 and 2 end before this append-only tail.
  void(TTX_CALL* resolve_domain)(ttx_abstract, ttx_domain_result);
  void(TTX_CALL* resolve_callable)(ttx_abstract, ttx_callable_result);
  void(TTX_CALL* resolve_route)(ttx_abstract, ttx_route_result);
  void(TTX_CALL* resolve_finite_extent)(ttx_abstract, ttx_finite_extent_result);
  void(TTX_CALL* resolve_bytes)(ttx_abstract, ttx_bytes_result);
};

// Abstract-valued operations are total and return a real handle through their
// sinks. name and Documentation are descriptive only. resolve_concept receives
// one complete byte route, including empty and non-textual routes, while
// visit_concepts advertises only the answers available during that call.

#define TTX_ABSTRACT_INTERFACE_PREFIX_SIZE \
  ((uint32_t)offsetof(ttx_abstract_ops, resolve_domain))
#define TTX_ABSTRACT_DOMAIN_PREFIX_SIZE \
  ((uint32_t)offsetof(ttx_abstract_ops, resolve_callable))
#define TTX_ABSTRACT_CALLABLE_PREFIX_SIZE \
  ((uint32_t)offsetof(ttx_abstract_ops, resolve_route))
#define TTX_ABSTRACT_ROUTE_PREFIX_SIZE \
  ((uint32_t)offsetof(ttx_abstract_ops, resolve_finite_extent))
#define TTX_ABSTRACT_EXTENT_PREFIX_SIZE \
  ((uint32_t)offsetof(ttx_abstract_ops, resolve_bytes))

struct ttx_layout_ops {
  ttx_abi_header header;
  void(TTX_CALL* fit)(
      ttx_layout,
      ttx_pack source,
      ttx_context context,
      ttx_pack_result);
  void(TTX_CALL* enumerable)(ttx_layout, ttx_enumerable_result);
  void(TTX_CALL* named)(ttx_layout, ttx_named_result);
  void(TTX_CALL* snapshot)(ttx_layout, ttx_layout_snapshot_result);
  void(TTX_CALL* fluid)(ttx_layout, ttx_fluid_result);
  void(TTX_CALL* value)(ttx_layout, ttx_value_layout_result);
  void(TTX_CALL* composite)(ttx_layout, ttx_composite_layout_result);
  void(TTX_CALL* ranged)(ttx_layout, ttx_ranged_layout_result);
  void(TTX_CALL* reindexed)(ttx_layout, ttx_reindexed_layout_result);
};

// Layout is an identity-free immutable projection. fit asks the receiving
// Layout to admit a source Pack and returns Unknown, None, or a
// Context-retained witness Pack. Enumerable and Named are support views over
// this exact snapshot rather than semantic categories or native casts.
//
// snapshot transfers one independently owned copy of the same observable
// Layout to its caller. The copy preserves fitting, support views, complete
// structure, paths, and borrowed producer identities without retaining parser
// state or another live source query. A Context releases that support value
// after every Pack which borrows it has ended.

struct ttx_layout_snapshot_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* layout)(ttx_layout_snapshot);
  void(TTX_CALL* release)(ttx_layout_snapshot);
};

struct ttx_callable_ops {
  ttx_abi_header header;
  ttx_abstract(TTX_CALL* candidate)(ttx_callable);
  ttx_layout(TTX_CALL* parameters)(ttx_callable);
  ttx_layout(TTX_CALL* results)(ttx_callable);
};

// Callable is an identity-free view of one exact candidate and its current
// parameter and result Layouts. It proves call shape without prescribing
// selection, invocation, execution, or a target calling convention.

struct ttx_fluid_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* layout)(ttx_fluid);
};

struct ttx_value_layout_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* layout)(ttx_value_layout);
  ttx_abstract(TTX_CALL* producer)(ttx_value_layout);
};

struct ttx_composite_layout_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* candidate)(ttx_composite_layout);
  ttx_layout(TTX_CALL* left)(ttx_composite_layout);
  ttx_layout(TTX_CALL* right)(ttx_composite_layout);
};

struct ttx_finite_extent_ops {
  ttx_abi_header header;
  ttx_abstract(TTX_CALL* candidate)(ttx_finite_extent);
  uint64_t(TTX_CALL* cardinality)(ttx_finite_extent);
};

struct ttx_ranged_layout_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* candidate)(ttx_ranged_layout);
  ttx_abstract(TTX_CALL* producer)(ttx_ranged_layout);
  ttx_abstract(TTX_CALL* extent)(ttx_ranged_layout);
};

struct ttx_reindexed_layout_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* candidate)(ttx_reindexed_layout);
  ttx_layout(TTX_CALL* source)(ttx_reindexed_layout);
  ttx_layout(TTX_CALL* projection)(ttx_reindexed_layout);
  void(TTX_CALL* visit_mappings)(ttx_reindexed_layout, ttx_reindex_sink);
};

struct ttx_bytes_ops {
  ttx_abi_header header;
  ttx_abstract(TTX_CALL* candidate)(ttx_bytes);
  uint64_t(TTX_CALL* size)(ttx_bytes);
  void(TTX_CALL* visit)(ttx_bytes, ttx_bytes_sink);
};

// Addressable asks each policy before it asks the referent. For Abstract-valued
// and typed resolution operations, None means that this policy does not own the
// question and traversal continues. Unknown or a resolved answer stops the
// current observation. Interface needs a separate pass arm because Rejected is
// itself a completed policy answer and must never fall through.
struct ttx_addressable_policy_ops {
  ttx_abi_header header;
  void(TTX_CALL* resolve_concept)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_borrowed_bytes route,
      ttx_abstract_sink);
  void(TTX_CALL* visit_concepts)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_concept_sink);
  void(TTX_CALL* interface)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_abstract requirement,
      ttx_addressable_interface_result);
  void(TTX_CALL* resolve_domain)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_domain_result);
  void(TTX_CALL* resolve_callable)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_callable_result);
  void(TTX_CALL* resolve_route)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_route_result);
  void(TTX_CALL* resolve_finite_extent)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_finite_extent_result);
  void(TTX_CALL* resolve_bytes)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_bytes_result);
  void(TTX_CALL* invoke)(
      ttx_addressable_policy,
      ttx_abstract candidate,
      ttx_abstract requirement,
      ttx_abstract operation,
      ttx_pack input,
      ttx_context,
      ttx_pack_result);
};

// A policy handle is borrowed by its Addressable for that graph lifetime. Its
// owner must therefore keep both the operation table and private dispatch state
// alive beside the Addressable. Plugin-backed policies satisfy that condition
// through Environment's retained provider lifetime rather than a TTX lease.

// Bytes is an identity-free view over one exact Abstract. It streams immutable
// chunks because a provider may live in another language or process and cannot
// promise that one native pointer is meaningful to its consumer. A concrete
// language may build text, buffers, images, or other value systems above this
// view without making byte sequences the universal TTX value domain.

// Fluid exposes a finite sequence whose producer positions can settle
// independently. Other Enumerable Layouts may retain stronger structure, so
// this view never flattens Composite boundaries or implies that every flow is
// positional.

struct ttx_route_ops {
  ttx_abi_header header;
  ttx_abstract(TTX_CALL* candidate)(ttx_route);
  ttx_borrowed_bytes(TTX_CALL* bytes)(ttx_route);
};

// Route is the narrow support view of one complete immutable concept name. Its
// bytes may be empty or non textual. The full sequence remains one atomic
// question and is never interpreted as a parameter by this contract.

struct ttx_pack_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* layout)(ttx_pack);
};

struct ttx_context_ops {
  ttx_abi_header header;
  void(TTX_CALL* release)(ttx_context);
  void(TTX_CALL* pack)(ttx_context, ttx_layout produced_flow, ttx_pack_result);
};

// pack copies the enumerable structure and paths while borrowing the exact
// producer Abstracts by accepting one Layout-owned snapshot. The caller
// supplies only produced-flow Layouts. release ends every Pack and releases
// every support snapshot retained by this Context.

struct ttx_enumerable_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* layout)(ttx_enumerable);
  uint64_t(TTX_CALL* cardinality)(ttx_enumerable);
  void(TTX_CALL* visit)(ttx_enumerable, ttx_layout_entry_sink);
};

struct ttx_named_ops {
  ttx_abi_header header;
  ttx_layout(TTX_CALL* candidate)(ttx_named);
  ttx_layout(TTX_CALL* source)(ttx_named);
  ttx_layout(TTX_CALL* routes)(ttx_named);
  void(TTX_CALL* visit_routes)(ttx_named, ttx_named_route_sink);
  void(TTX_CALL* select)(
      ttx_named,
      ttx_borrowed_bytes complete_route,
      ttx_named_selection_result);
};

// Exact identity is useful only when the caller intends to compare graph
// subjects. Equal capabilities identify the same subject; unequal handles say
// nothing about compatible Domains, fitting, Interface satisfaction, or
// behavioral equivalence.
static inline uint8_t ttx_abstract_same(ttx_abstract left, ttx_abstract right) {
  return (uint8_t)(left == right);
}

TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_unknown(void);
TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_none(void);
TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_constant_requirement(void);
TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_addressable_requirement(void);
TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_callable_requirement(void);
TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_route_requirement(void);
TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_finite_extent_requirement(void);
TTX_EXTERN_C TTX_API ttx_abstract TTX_CALL ttx_bytes_requirement(void);
TTX_EXTERN_C TTX_API ttx_layout TTX_CALL ttx_empty_layout(void);

// The reference Context owns every Pack and transferred Layout snapshot until
// release. Allocation failure returns the all-zero invalid support handle.
TTX_EXTERN_C TTX_API ttx_context TTX_CALL ttx_context_create(void);

#undef TTX_DECLARE_HANDLE

#endif
