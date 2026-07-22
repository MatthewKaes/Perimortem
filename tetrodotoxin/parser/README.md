# Tetrodotoxin Parser Revival

`tetrodotoxin/parser` restores the parser-first frontend that preceded the ISA
and interpreter experiments. The historical authority is split across two
adjacent revisions:

- `7001442a^` contains the last working recursive Type parser and its focused
  specialization and recovery tests.
- `7001442a` introduces the original Abstract-context design: lexical
  classification permits progressive local resolution, and parsing enriches
  real Abstract owners instead of constructing a parallel compiler model. Its
  newly moved `Type::parse` is a stub, so this revision supplies ownership
  authority rather than an implementation to copy.

The older algorithm is migrated to the current `Ttx::Lexical` and
`Ttx::Concept::Abstract` contracts. Its obsolete Tokenizer, nullable Abstract
pointers, handler enum, standard-library registry, and copied Type tree are not
restored.

## Owner contract

The parser owns deterministic consumption of TTX token bytecode and progressive
resolution against the real Abstract context supplied by its caller. It does
not own source bytes, semantic identities, package loading, target lowering, or
runtime execution.

- Source and its Tokenizer own input lifetime.
- The parser borrows a `Ttx::Lexical::Cursor` and reports through its Errors.
- Tetrodotoxin owns immutable scalar Type identities indexed by
  `Perimortem::Utility::Table`. `Bool`, the fixed-width integer and real Types,
  and the resolved `Count` alias use this fast path.
- The caller supplies the first real Abstract context for every other name.
- Each authored segment resolves from the Abstract selected by the preceding
  segment.
- Graph queries still fail with the one `Ttx::Concept::Invalid` object. The
  parser consumes that result, records its diagnostic, and returns
  `Perimortem::Utility::None` because parse failure is not a semantic graph
  identity.
- A Type parser returns `Option<const Ttx::Model::Type&>`, borrowing the real
  resolved Type identity. It never returns a copied Type description or
  syntax-only Type graph.
- A Generic publishes its complete ordered parameter signature. The parser
  validates nested `const Type&`, `Unsigned_64`, and `Bool` arguments and passes
  their compact Union view to the formula's cache. Tetrodotoxin's shared
  Environment owns the `View` and `Access` formulas; the builtin fast path
  contains only concrete scalar Types.
- Parsing is left-to-right and never backtracks.

## Revival slices

1. Restore progressive non-generic Type-reference parsing and prove direct,
   nested, missing, and wrong-contract behavior.
2. Restore generic arguments using the Generic's declared parameter signature,
   compact const-Type/scalar Union values, cache-owned materialized Types, and
   sequence-point recovery. Environment-owned `View` and `Access` establish
   this slice across every Source in one interpretation transaction.
3. Restore definitions, aliases, comments, attributes, and declaration
   recovery onto their real owners.
4. Restore functions, layouts, packs, expressions, and statements one grammar
   family at a time with focused historical behavior tests.
5. Restore the Source envelope and package parser, then connect one canonical
   authored Source to the existing model owners.
6. Replace each active evaluator consumer only after parser output passes an
   independent behavior comparison. Delete the superseded path after the last
   consumer moves.

Each slice must build `//tetrodotoxin:parser`, add focused behavior coverage,
and leave later grammar visibly unsupported rather than fabricating success.
