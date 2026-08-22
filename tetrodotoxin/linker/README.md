# Linker

Linker owns source-independent native products and the agreements needed to
combine them. Library and other CPU compilers produce object inputs, while
Linker retains symbols, relocations, target encoding, imports, and final ELF or
PE products without consulting the semantic graph that produced them.

CPU target and operating-system host remain separate choices. A CPU target
selects instruction set, data layout, calling convention, and object carrier
rules. A host selects process entry, loader behavior, runtime and System
providers, dynamic dependencies, and executable format.

## ABI manifest

Every Package native artifact has one ABI Manifest beside its native bytes.
The Manifest records:

- Package identity and version
- logical artifact and target identifiers
- one ABI fingerprint
- every selected native import and its logical provider

The fingerprint covers the target data layout, generated C carrier surface,
published native signatures, calling conventions, and unresolved imported
signatures. It detects stale or mismatched build products. It is not a security
or content-integrity digest.

Package Archives retain the same artifact agreement. Repository and
source-free application selection compare the Archive and Manifest before a
native path can be accepted. Moving an artifact changes neither semantic nor
ABI identity because filesystem paths remain build declarations outside both
formats.

## Imports and providers

A compiler publishes every unresolved Foreign State and Function as an Import.
The Import retains its ABI, native symbol, and category. It does not select a
filesystem object or platform implementation.

The build request supplies target-specific Provider records. Provider selection
matches target, ABI, category, and symbol together and requires exactly one
answer. Package compilation records that logical provider in the artifact
agreement. Missing and ambiguous providers fail before an Archive or native
artifact is published.

This lets Linux and Windows providers implement one stable TTX-facing C surface
without making Foreign declarations conditional. A platform package may still
declare distinct low-level imports when the native APIs genuinely have
different signatures.

## Generated C interfaces

Generated aggregate names begin with lowercase `ttx_`, followed by a lowercase
Package coordinate and the semantic member and Type route. For example:

```c
ttx_perimortem_memory_Dynamic_Bytes
```

Package and semantic separators remain readable underscores. Source underscores
and other nonalphanumeric bytes use hexadecimal escapes, so two distinct routes
cannot collapse merely because punctuation was removed. A generated header
includes dependency owner headers and never redeclares an imported carrier.
Per-carrier guards let several generated headers share the same canonical owner
definition safely.

## Native production

Linker consumes ELF and COFF object and archive inputs, resolves their declared
symbols and imports, performs archive extraction, applies relocations, and emits
ELF or PE products for the selected host. LLVM and a direct Library compiler
are alternative object producers. Linker depends on their source-independent
object contracts, never on LLVM or a copied Library graph.
