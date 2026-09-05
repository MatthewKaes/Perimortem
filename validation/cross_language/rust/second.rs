// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

use std::sync::Arc;

use super::super::ttx;

#[repr(C)]
#[derive(Copy, Clone)]
pub struct PolicyExports {
    pub candidate: ttx::Abstract,
    pub requirement: ttx::Abstract,
    pub operation: ttx::Abstract,
}

#[no_mangle]
pub extern "C" fn rust_policy_exports() -> PolicyExports {
    let operation = ttx::retain_abstract(ttx::AbstractModel::plain(b"permit"));
    let mut requirement = ttx::AbstractModel::plain(b"Visibility");
    requirement.concepts.push((b"permit", operation));
    let requirement = ttx::retain_abstract(requirement);

    let mut candidate = ttx::AbstractModel::plain(b"rust_visibility_policy");
    candidate.domain = Arc::new(|_| ttx::DomainProjection::Unknown);
    candidate.interface = Arc::new(move |_, requested| {
        if !ttx::same(requested, requirement) {
            return ttx::InterfaceModel::rejected();
        }
        let invoke: ttx::Invocation = Arc::new(
            move |selected: ttx::Abstract,
                  input: ttx::Pack,
                  context: ttx::Context,
                  result: ttx::PackResult| {
                if !ttx::same(selected, operation) {
                    unsafe { ((*result.operations).none)(result) }
                    return;
                }
                let layout = unsafe { ((*input.operations).layout)(input) };
                unsafe { ((*context.operations).pack)(context, layout, result) }
            },
        );
        ttx::InterfaceModel {
            relation: ttx::InterfaceRelation::Satisfied,
            invoke: Some(invoke),
        }
    });
    let candidate = ttx::retain_abstract(candidate);
    if ttx::relation(candidate, requirement) != ttx::InterfaceRelation::Satisfied {
        std::process::abort();
    }

    PolicyExports {
        candidate,
        requirement,
        operation,
    }
}
