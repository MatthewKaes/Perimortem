// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

use super::super::ttx;

// This producer owns no permanent Layout. Context must copy its projection
// during the callback before the local Rust vectors are dropped.
#[no_mangle]
pub extern "C" fn rust_temporary_pack(
    producer: ttx::Abstract,
    context: ttx::Context,
    result: ttx::PackResult,
) {
    ttx::with_layout(
        ttx::LayoutModel {
            entries: vec![(vec![0], producer)],
            receiving_domains: None,
        },
        |layout| unsafe { ((*context.operations).pack)(context, layout, result) },
    );
}
