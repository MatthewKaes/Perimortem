// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

use std::io::Write;
use std::sync::{Arc, OnceLock};

use super::ttx;

pub enum BytesTerminalSelf {}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct BytesTerminal {
    pub operations: *const BytesTerminalOps,
    pub self_: *mut BytesTerminalSelf,
}

unsafe impl Send for BytesTerminal {}
unsafe impl Sync for BytesTerminal {}

#[repr(C)]
pub struct BytesTerminalOps {
    pub header: ttx::AbiHeader,
    pub project: extern "C" fn(BytesTerminal, ttx::Abstract, ttx::Abstract, BytesSink),
}

pub enum BytesSinkSelf {}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct BytesSink {
    pub operations: *const BytesSinkOps,
    pub self_: *mut BytesSinkSelf,
}

#[repr(C)]
pub struct BytesSinkOps {
    pub header: ttx::AbiHeader,
    pub rejected: extern "C" fn(BytesSink),
    pub projected: extern "C" fn(BytesSink, ttx::BorrowedBytes),
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct Exports {
    pub echo: ttx::Abstract,
    pub view_bytes: ttx::Abstract,
    pub value: ttx::Abstract,
}

#[derive(Copy, Clone)]
struct Contracts {
    echo: ttx::Abstract,
    view_bytes: ttx::Abstract,
    value: ttx::Abstract,
    print: ttx::Abstract,
    to_string: ttx::Abstract,
    terminal: BytesTerminal,
}

#[repr(C)]
struct Projection {
    operations: BytesSinkOps,
    context: ttx::Context,
    result: ttx::PackResult,
}

#[repr(C)]
struct ValueCall {
    operations: ttx::PackResultOps,
    contracts: Contracts,
    context: ttx::Context,
    result: ttx::PackResult,
}

static CONTRACTS: OnceLock<Contracts> = OnceLock::new();

fn projection(value: BytesSink) -> &'static Projection {
    if value.operations.is_null() {
        std::process::abort();
    }
    unsafe { &*value.self_.cast::<Projection>() }
}

extern "C" fn bytes_rejected(value: BytesSink) {
    let projection = projection(value);
    unsafe { ((*projection.result.operations).none)(projection.result) }
}

extern "C" fn bytes_projected(value: BytesSink, bytes: ttx::BorrowedBytes) {
    let projection = projection(value);
    if bytes.size != 0 && (bytes.data.is_null() || bytes.size > usize::MAX as u64) {
        unsafe { ((*projection.result.operations).none)(projection.result) }
        return;
    }
    let payload = if bytes.size == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(bytes.data, bytes.size as usize) }
    };
    let mut output = std::io::stdout().lock();
    output.write_all(b"Hello from [Rust]: ").unwrap();
    output.write_all(payload).unwrap();
    output.write_all(b"\n").unwrap();
    unsafe {
        ((*projection.context.operations).pack)(
            projection.context,
            ttx::empty_layout(),
            projection.result,
        )
    }
}

fn value_call(value: ttx::PackResult) -> &'static ValueCall {
    if value.operations.is_null() {
        std::process::abort();
    }
    unsafe { &*value.self_.cast::<ValueCall>() }
}

extern "C" fn value_unknown(value: ttx::PackResult) {
    let call = value_call(value);
    unsafe { ((*call.result.operations).unknown)(call.result) }
}

extern "C" fn value_none(value: ttx::PackResult) {
    let call = value_call(value);
    unsafe { ((*call.result.operations).none)(call.result) }
}

extern "C" fn value_support_failed(value: ttx::PackResult, failure: ttx::PackSupportFailure) {
    let call = value_call(value);
    unsafe { ((*call.result.operations).support_failed)(call.result, failure) }
}

extern "C" fn value_packed(value: ttx::PackResult, pack: ttx::Pack) {
    let call = value_call(value);
    let producer = match ttx::admit(pack, &[call.contracts.view_bytes]) {
        ttx::Admission::SupportFailed => {
            unsafe {
                ((*call.result.operations).support_failed)(
                    call.result,
                    ttx::PackSupportFailure::InvalidLayout,
                )
            }
            return;
        }
        ttx::Admission::Unknown => {
            unsafe { ((*call.result.operations).unknown)(call.result) }
            return;
        }
        ttx::Admission::Rejected => {
            unsafe { ((*call.result.operations).none)(call.result) }
            return;
        }
        ttx::Admission::Exact { producers, .. } => producers[0],
    };
    let projection = Projection {
        operations: BytesSinkOps {
            header: ttx::AbiHeader {
                size: std::mem::size_of::<BytesSinkOps>() as u32,
                abi_major: ttx::ABI_MAJOR,
                abi_minor: ttx::ABI_MINOR,
            },
            rejected: bytes_rejected,
            projected: bytes_projected,
        },
        context: call.context,
        result: call.result,
    };
    let callback = BytesSink {
        operations: &projection.operations,
        self_: (&projection as *const Projection)
            .cast_mut()
            .cast::<BytesSinkSelf>(),
    };
    unsafe {
        ((*call.contracts.terminal.operations).project)(
            call.contracts.terminal,
            producer,
            call.contracts.view_bytes,
            callback,
        )
    }
}

fn invoke_stored_echo(
    contracts: Contracts,
    value: ttx::Abstract,
    operation: ttx::Abstract,
    input: ttx::Pack,
    context: ttx::Context,
    result: ttx::PackResult,
) {
    if !ttx::same(operation, contracts.print) {
        unsafe { ((*result.operations).none)(result) }
        return;
    }
    match ttx::admit(input, &[]) {
        ttx::Admission::Exact { .. } => {}
        ttx::Admission::Unknown => {
            unsafe { ((*result.operations).unknown)(result) }
            return;
        }
        ttx::Admission::Rejected => {
            unsafe { ((*result.operations).none)(result) }
            return;
        }
        ttx::Admission::SupportFailed => {
            unsafe {
                ((*result.operations).support_failed)(
                    result,
                    ttx::PackSupportFailure::InvalidLayout,
                )
            }
            return;
        }
    }
    let call = ValueCall {
        operations: ttx::PackResultOps {
            header: ttx::AbiHeader {
                size: std::mem::size_of::<ttx::PackResultOps>() as u32,
                abi_major: ttx::ABI_MAJOR,
                abi_minor: ttx::ABI_MINOR,
            },
            unknown: value_unknown,
            none: value_none,
            packed: value_packed,
            support_failed: value_support_failed,
        },
        contracts,
        context,
        result,
    };
    let callback = ttx::PackResult {
        operations: &call.operations,
        self_: (&call as *const ValueCall)
            .cast_mut()
            .cast::<ttx::PackResultSelf>(),
    };
    ttx::invoke(
        value,
        contracts.value,
        contracts.to_string,
        input,
        context,
        callback,
    );
}

pub fn create(terminal: BytesTerminal) -> Exports {
    if terminal.operations.is_null() {
        std::process::abort();
    }

    let print = ttx::retain_abstract(ttx::AbstractModel::plain(b"print"));
    let to_string = ttx::retain_abstract(ttx::AbstractModel::plain(b"to_string"));

    let mut echo = ttx::AbstractModel::plain(b"Echo");
    echo.concepts.push((b"print", print));
    echo.domain = Arc::new(|_| ttx::DomainProjection::SelfWith(ttx::empty_layout()));
    let echo = ttx::retain_abstract(echo);

    let mut value = ttx::AbstractModel::plain(b"Value");
    value.concepts.push((b"to_string", to_string));
    let value = ttx::retain_abstract(value);

    let view_layout = Arc::new(OnceLock::new());
    let view_layout_for_domain = Arc::clone(&view_layout);
    let mut view = ttx::AbstractModel::plain(b"View::Bytes");
    view.domain = Arc::new(move |_| {
        ttx::DomainProjection::SelfWith(
            *view_layout_for_domain
                .get()
                .unwrap_or_else(|| std::process::abort()),
        )
    });
    let view = ttx::retain_abstract(view);
    let layout = ttx::retain_layout(ttx::LayoutModel {
        entries: vec![(vec![b'0'], view)],
        receiving_domains: Some(vec![view]),
    });
    view_layout
        .set(layout)
        .unwrap_or_else(|_| std::process::abort());

    CONTRACTS
        .set(Contracts {
            echo,
            view_bytes: view,
            value,
            print,
            to_string,
            terminal,
        })
        .unwrap_or_else(|_| std::process::abort());

    Exports {
        echo,
        view_bytes: view,
        value,
    }
}

#[no_mangle]
pub extern "C" fn rust_echo_create(value: ttx::Abstract) -> ttx::Abstract {
    let contracts = *CONTRACTS.get().unwrap_or_else(|| std::process::abort());
    let value_relation = ttx::relation(value, contracts.value);
    let bytes_relation = ttx::relation(value, contracts.view_bytes);
    if value_relation != ttx::InterfaceRelation::Satisfied
        && value_relation != ttx::InterfaceRelation::Equivalent
    {
        return ttx::unknown();
    }
    let viewable = bytes_relation == ttx::InterfaceRelation::Satisfied
        || bytes_relation == ttx::InterfaceRelation::Equivalent;

    let mut model = ttx::AbstractModel::plain(b"rust_stored_echo");
    model.concepts.push((b"value", value));
    model.domain = Arc::new(|_| ttx::DomainProjection::Unknown);
    model.bytes = Arc::new(move |candidate, result| ttx::forward_bytes(value, candidate, result));
    model.interface = Arc::new(move |_, requirement| {
        if ttx::same(requirement, contracts.echo) {
            let invoke: ttx::Invocation = Arc::new(
                move |operation: ttx::Abstract,
                      input: ttx::Pack,
                      context: ttx::Context,
                      result: ttx::PackResult| {
                    invoke_stored_echo(contracts, value, operation, input, context, result)
                },
            );
            ttx::InterfaceModel {
                relation: ttx::InterfaceRelation::Satisfied,
                invoke: Some(invoke),
            }
        } else if (ttx::same(requirement, contracts.view_bytes)
            || ttx::same(requirement, ttx::bytes_requirement()))
            && viewable
        {
            ttx::InterfaceModel {
                relation: ttx::InterfaceRelation::Satisfied,
                invoke: None,
            }
        } else {
            ttx::InterfaceModel::rejected()
        }
    });
    let candidate = ttx::retain_abstract(model);
    candidate
}
