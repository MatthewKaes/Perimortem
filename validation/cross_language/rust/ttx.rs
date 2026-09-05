// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

use std::sync::Arc;

pub const ABI_MAJOR: u16 = 1;
pub const ABI_MINOR: u16 = 0;

#[repr(C)]
#[derive(Copy, Clone)]
pub struct AbiHeader {
    pub size: u32,
    pub abi_major: u16,
    pub abi_minor: u16,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct BorrowedBytes {
    pub data: *const u8,
    pub size: u64,
}

macro_rules! handle {
    ($name:ident, $ops:ident, $self:ident) => {
        pub enum $self {}

        #[repr(C)]
        #[derive(Copy, Clone)]
        pub struct $name {
            pub operations: *const $ops,
            pub self_: *mut $self,
        }

        unsafe impl Send for $name {}
        unsafe impl Sync for $name {}
    };
}

#[repr(C)]
pub struct AbstractCapability {
    pub operations: *const AbstractOps,
}
#[repr(transparent)]
#[derive(Copy, Clone)]
pub struct Abstract {
    pub capability: *const AbstractCapability,
}
unsafe impl Send for Abstract {}
unsafe impl Sync for Abstract {}
impl Abstract {
    pub unsafe fn ops<'call>(self) -> &'call AbstractOps {
        assert!(!self.capability.is_null());
        unsafe { &*(*self.capability).operations }
    }
}
handle!(Documentation, DocumentationOps, DocumentationSelf);
#[repr(C)]
pub struct InterfaceCapability {
    pub operations: *const InterfaceOps,
}
#[repr(transparent)]
#[derive(Copy, Clone)]
pub struct Interface {
    pub capability: *const InterfaceCapability,
}
impl Interface {
    unsafe fn ops<'call>(self) -> &'call InterfaceOps {
        assert!(!self.capability.is_null());
        unsafe { &*(*self.capability).operations }
    }
}
handle!(Layout, LayoutOps, LayoutSelf);
handle!(Pack, PackOps, PackSelf);
handle!(Context, ContextOps, ContextSelf);
handle!(Enumerable, EnumerableOps, EnumerableSelf);
handle!(Named, NamedOps, NamedSelf);
handle!(LayoutSnapshot, LayoutSnapshotOps, LayoutSnapshotSelf);
handle!(Callable, CallableOps, CallableSelf);
handle!(Fluid, FluidOps, FluidSelf);
handle!(Route, RouteOps, RouteSelf);
handle!(ValueLayout, ValueLayoutOps, ValueLayoutSelf);
handle!(CompositeLayout, CompositeLayoutOps, CompositeLayoutSelf);
handle!(Extent, ExtentOps, ExtentSelf);
handle!(RangedLayout, RangedLayoutOps, RangedLayoutSelf);
handle!(ReindexedLayout, ReindexedLayoutOps, ReindexedLayoutSelf);
handle!(Bytes, BytesOps, BytesSelf);
handle!(BytesSink, BytesSinkOps, BytesSinkSelf);
handle!(AbstractSink, AbstractSinkOps, AbstractSinkSelf);
handle!(ConceptSink, ConceptSinkOps, ConceptSinkSelf);
handle!(InterfaceSink, InterfaceSinkOps, InterfaceSinkSelf);
handle!(DomainResult, DomainResultOps, DomainResultSelf);
handle!(PackResult, PackResultOps, PackResultSelf);
handle!(EnumerableResult, EnumerableResultOps, EnumerableResultSelf);
handle!(NamedResult, NamedResultOps, NamedResultSelf);
handle!(
    LayoutSnapshotResult,
    LayoutSnapshotResultOps,
    LayoutSnapshotResultSelf
);
handle!(CallableResult, CallableResultOps, CallableResultSelf);
handle!(FluidResult, FluidResultOps, FluidResultSelf);
handle!(RouteResult, RouteResultOps, RouteResultSelf);
handle!(
    ValueLayoutResult,
    ValueLayoutResultOps,
    ValueLayoutResultSelf
);
handle!(
    CompositeLayoutResult,
    CompositeLayoutResultOps,
    CompositeLayoutResultSelf
);
handle!(ExtentResult, ExtentResultOps, ExtentResultSelf);
handle!(
    RangedLayoutResult,
    RangedLayoutResultOps,
    RangedLayoutResultSelf
);
handle!(
    ReindexedLayoutResult,
    ReindexedLayoutResultOps,
    ReindexedLayoutResultSelf
);
handle!(BytesResult, BytesResultOps, BytesResultSelf);
handle!(
    NamedSelectionResult,
    NamedSelectionResultOps,
    NamedSelectionResultSelf
);
handle!(LayoutEntrySink, LayoutEntrySinkOps, LayoutEntrySinkSelf);
handle!(NamedRouteSink, NamedRouteSinkOps, NamedRouteSinkSelf);
handle!(ReindexSink, ReindexSinkOps, ReindexSinkSelf);

#[repr(C)]
#[derive(Copy, Clone, Eq, PartialEq)]
pub enum InterfaceRelation {
    Unknown = 0,
    Rejected = 1,
    Satisfied = 2,
    Equivalent = 3,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub enum PackSupportFailure {
    InvalidLayout = 1,
    Exhausted = 2,
}

#[repr(C)]
pub struct BytesSinkOps {
    pub header: AbiHeader,
    pub bytes: extern "C" fn(BytesSink, BorrowedBytes),
    pub completed: extern "C" fn(BytesSink),
}

#[repr(C)]
pub struct AbstractSinkOps {
    pub header: AbiHeader,
    pub answer: extern "C" fn(AbstractSink, Abstract),
}

#[repr(C)]
pub struct ConceptSinkOps {
    pub header: AbiHeader,
    pub item: extern "C" fn(ConceptSink, BorrowedBytes, Abstract),
    pub completed: extern "C" fn(ConceptSink),
}

#[repr(C)]
pub struct InterfaceSinkOps {
    pub header: AbiHeader,
    pub answer: extern "C" fn(InterfaceSink, Interface),
}

#[repr(C)]
pub struct DomainResultOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(DomainResult),
    pub none: extern "C" fn(DomainResult),
    pub resolved: extern "C" fn(DomainResult, Abstract, Layout),
}

#[repr(C)]
pub struct PackResultOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(PackResult),
    pub none: extern "C" fn(PackResult),
    pub packed: extern "C" fn(PackResult, Pack),
    pub support_failed: extern "C" fn(PackResult, PackSupportFailure),
}

#[repr(C)]
pub struct BytesResultOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(BytesResult),
    pub none: extern "C" fn(BytesResult),
    pub resolved: extern "C" fn(BytesResult, Bytes),
}

#[repr(C)]
pub struct EnumerableResultOps {
    pub header: AbiHeader,
    pub rejected: extern "C" fn(EnumerableResult),
    pub satisfied: extern "C" fn(EnumerableResult, Enumerable),
}

#[repr(C)]
pub struct NamedResultOps {
    pub header: AbiHeader,
    pub rejected: extern "C" fn(NamedResult),
    pub satisfied: extern "C" fn(NamedResult, Named),
}

#[repr(C)]
pub struct LayoutSnapshotResultOps {
    pub header: AbiHeader,
    pub retained: extern "C" fn(LayoutSnapshotResult, LayoutSnapshot),
    pub support_failed: extern "C" fn(LayoutSnapshotResult, PackSupportFailure),
}

#[repr(C)]
pub struct CallableResultOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(CallableResult),
    pub none: extern "C" fn(CallableResult),
    pub resolved: extern "C" fn(CallableResult, Callable),
    pub support_failed: extern "C" fn(CallableResult, PackSupportFailure),
}

#[repr(C)]
pub struct FluidResultOps {
    pub header: AbiHeader,
    pub rejected: extern "C" fn(FluidResult),
    pub satisfied: extern "C" fn(FluidResult, Fluid),
}

#[repr(C)]
pub struct RouteResultOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(RouteResult),
    pub none: extern "C" fn(RouteResult),
    pub resolved: extern "C" fn(RouteResult, Route),
}

#[repr(C)]
pub struct ValueLayoutResultOps {
    pub header: AbiHeader,
    pub rejected: extern "C" fn(ValueLayoutResult),
    pub satisfied: extern "C" fn(ValueLayoutResult, ValueLayout),
}

#[repr(C)]
pub struct CompositeLayoutResultOps {
    pub header: AbiHeader,
    pub rejected: extern "C" fn(CompositeLayoutResult),
    pub satisfied: extern "C" fn(CompositeLayoutResult, CompositeLayout),
}

#[repr(C)]
pub struct ExtentResultOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(ExtentResult),
    pub none: extern "C" fn(ExtentResult),
    pub resolved: extern "C" fn(ExtentResult, Extent),
}

#[repr(C)]
pub struct RangedLayoutResultOps {
    pub header: AbiHeader,
    pub rejected: extern "C" fn(RangedLayoutResult),
    pub satisfied: extern "C" fn(RangedLayoutResult, RangedLayout),
}

#[repr(C)]
pub struct ReindexedLayoutResultOps {
    pub header: AbiHeader,
    pub rejected: extern "C" fn(ReindexedLayoutResult),
    pub satisfied: extern "C" fn(ReindexedLayoutResult, ReindexedLayout),
}

#[repr(C)]
pub struct NamedSelectionResultOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(NamedSelectionResult),
    pub none: extern "C" fn(NamedSelectionResult),
    pub selected: extern "C" fn(NamedSelectionResult, BorrowedBytes),
}

#[repr(C)]
pub struct LayoutEntrySinkOps {
    pub header: AbiHeader,
    pub entry: extern "C" fn(LayoutEntrySink, BorrowedBytes, Abstract),
    pub completed: extern "C" fn(LayoutEntrySink),
}

#[repr(C)]
pub struct NamedRouteSinkOps {
    pub header: AbiHeader,
    pub unknown: extern "C" fn(NamedRouteSink, BorrowedBytes),
    pub none: extern "C" fn(NamedRouteSink, BorrowedBytes),
    pub route: extern "C" fn(NamedRouteSink, BorrowedBytes, BorrowedBytes),
    pub completed: extern "C" fn(NamedRouteSink),
}

#[repr(C)]
pub struct ReindexSinkOps {
    pub header: AbiHeader,
    pub mapping: extern "C" fn(ReindexSink, BorrowedBytes, BorrowedBytes),
    pub completed: extern "C" fn(ReindexSink),
}

#[repr(C)]
pub struct DocumentationOps {
    pub header: AbiHeader,
    pub size: extern "C" fn(Documentation) -> u64,
    pub visit_bytes: extern "C" fn(Documentation, BytesSink),
}

#[repr(C)]
pub struct InterfaceOps {
    pub header: AbiHeader,
    pub requirement: extern "C" fn(Interface) -> Abstract,
    pub candidate: extern "C" fn(Interface) -> Abstract,
    pub negotiate: extern "C" fn(Interface) -> InterfaceRelation,
    pub invoke: extern "C" fn(Interface, Abstract, Pack, Context, PackResult),
}

#[repr(C)]
pub struct AbstractOps {
    pub header: AbiHeader,
    pub name: extern "C" fn(Abstract) -> BorrowedBytes,
    pub documentation: extern "C" fn(Abstract) -> Documentation,
    pub resolve: extern "C" fn(Abstract, AbstractSink),
    pub resolve_concept: extern "C" fn(Abstract, BorrowedBytes, AbstractSink),
    pub visit_concepts: extern "C" fn(Abstract, ConceptSink),
    pub interface: extern "C" fn(Abstract, Abstract, InterfaceSink),
    pub resolve_domain: extern "C" fn(Abstract, DomainResult),
    pub resolve_callable: extern "C" fn(Abstract, CallableResult),
    pub resolve_route: extern "C" fn(Abstract, RouteResult),
    pub resolve_finite_extent: extern "C" fn(Abstract, ExtentResult),
    pub resolve_bytes: extern "C" fn(Abstract, BytesResult),
}

#[repr(C)]
pub struct LayoutOps {
    pub header: AbiHeader,
    pub fit: extern "C" fn(Layout, Pack, Context, PackResult),
    pub enumerable: extern "C" fn(Layout, EnumerableResult),
    pub named: extern "C" fn(Layout, NamedResult),
    pub snapshot: extern "C" fn(Layout, LayoutSnapshotResult),
    pub fluid: extern "C" fn(Layout, FluidResult),
    pub value: extern "C" fn(Layout, ValueLayoutResult),
    pub composite: extern "C" fn(Layout, CompositeLayoutResult),
    pub ranged: extern "C" fn(Layout, RangedLayoutResult),
    pub reindexed: extern "C" fn(Layout, ReindexedLayoutResult),
}

#[repr(C)]
pub struct LayoutSnapshotOps {
    pub header: AbiHeader,
    pub layout: extern "C" fn(LayoutSnapshot) -> Layout,
    pub release: extern "C" fn(LayoutSnapshot),
}

#[repr(C)]
pub struct CallableOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(Callable) -> Abstract,
    pub parameters: extern "C" fn(Callable) -> Layout,
    pub results: extern "C" fn(Callable) -> Layout,
}

#[repr(C)]
pub struct FluidOps {
    pub header: AbiHeader,
    pub layout: extern "C" fn(Fluid) -> Layout,
}

#[repr(C)]
pub struct ValueLayoutOps {
    pub header: AbiHeader,
    pub layout: extern "C" fn(ValueLayout) -> Layout,
    pub producer: extern "C" fn(ValueLayout) -> Abstract,
}

#[repr(C)]
pub struct CompositeLayoutOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(CompositeLayout) -> Layout,
    pub left: extern "C" fn(CompositeLayout) -> Layout,
    pub right: extern "C" fn(CompositeLayout) -> Layout,
}

#[repr(C)]
pub struct ExtentOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(Extent) -> Abstract,
    pub cardinality: extern "C" fn(Extent) -> u64,
}

#[repr(C)]
pub struct RangedLayoutOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(RangedLayout) -> Layout,
    pub producer: extern "C" fn(RangedLayout) -> Abstract,
    pub extent: extern "C" fn(RangedLayout) -> Abstract,
}

#[repr(C)]
pub struct ReindexedLayoutOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(ReindexedLayout) -> Layout,
    pub source: extern "C" fn(ReindexedLayout) -> Layout,
    pub projection: extern "C" fn(ReindexedLayout) -> Layout,
    pub visit_mappings: extern "C" fn(ReindexedLayout, ReindexSink),
}

#[repr(C)]
pub struct BytesOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(Bytes) -> Abstract,
    pub size: extern "C" fn(Bytes) -> u64,
    pub visit: extern "C" fn(Bytes, BytesSink),
}

#[repr(C)]
pub struct RouteOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(Route) -> Abstract,
    pub bytes: extern "C" fn(Route) -> BorrowedBytes,
}

#[repr(C)]
pub struct PackOps {
    pub header: AbiHeader,
    pub layout: extern "C" fn(Pack) -> Layout,
}

#[repr(C)]
pub struct ContextOps {
    pub header: AbiHeader,
    pub release: extern "C" fn(Context),
    pub pack: extern "C" fn(Context, Layout, PackResult),
}

#[repr(C)]
pub struct EnumerableOps {
    pub header: AbiHeader,
    pub layout: extern "C" fn(Enumerable) -> Layout,
    pub cardinality: extern "C" fn(Enumerable) -> u64,
    pub visit: extern "C" fn(Enumerable, LayoutEntrySink),
}

#[repr(C)]
pub struct NamedOps {
    pub header: AbiHeader,
    pub candidate: extern "C" fn(Named) -> Layout,
    pub source: extern "C" fn(Named) -> Layout,
    pub routes: extern "C" fn(Named) -> Layout,
    pub visit_routes: extern "C" fn(Named, NamedRouteSink),
    pub select: extern "C" fn(Named, BorrowedBytes, NamedSelectionResult),
}

pub type InterfaceFactory = Arc<dyn Fn(Abstract, Abstract) -> InterfaceModel + Send + Sync>;
pub type Invocation = Arc<dyn Fn(Abstract, Pack, Context, PackResult) + Send + Sync>;
pub type DomainFactory = Arc<dyn Fn(Abstract) -> DomainProjection + Send + Sync>;

#[derive(Clone)]
pub enum DomainProjection {
    Unknown,
    None,
    SelfWith(Layout),
}

#[derive(Clone)]
pub struct InterfaceModel {
    pub relation: InterfaceRelation,
    pub invoke: Option<Invocation>,
}

impl InterfaceModel {
    pub fn rejected() -> Self {
        Self {
            relation: InterfaceRelation::Rejected,
            invoke: None,
        }
    }
}

#[repr(C)]
struct AbstractBinding {
    capability: AbstractCapability,
    model: AbstractModel,
}

pub struct AbstractModel {
    pub name: &'static [u8],
    pub documentation: &'static [u8],
    pub concepts: Vec<(&'static [u8], Abstract)>,
    pub interface: InterfaceFactory,
    pub domain: DomainFactory,
    pub bytes: Arc<dyn Fn(Abstract, BytesResult) + Send + Sync>,
}

impl AbstractModel {
    pub fn plain(name: &'static [u8]) -> Self {
        Self {
            name,
            documentation: b"",
            concepts: Vec::new(),
            interface: Arc::new(|_, _| InterfaceModel::rejected()),
            domain: Arc::new(|_| DomainProjection::None),
            bytes: Arc::new(|_, result| unsafe { ((*result.operations).none)(result) }),
        }
    }
}

#[derive(Clone)]
pub struct LayoutModel {
    pub entries: Vec<(Vec<u8>, Abstract)>,
    pub receiving_domains: Option<Vec<Abstract>>,
}

pub fn retain_abstract(model: AbstractModel) -> Abstract {
    let binding = Box::into_raw(Box::new(AbstractBinding {
        capability: AbstractCapability {
            operations: &ABSTRACT_OPS,
        },
        model,
    }));
    Abstract {
        capability: unsafe { &(*binding).capability },
    }
}

pub fn with_layout(model: LayoutModel, use_layout: impl FnOnce(Layout)) {
    let mut model = model;
    use_layout(Layout {
        operations: &LAYOUT_OPS,
        self_: (&mut model as *mut LayoutModel).cast(),
    });
}

pub fn retain_layout(model: LayoutModel) -> Layout {
    let model = Box::into_raw(Box::new(model));
    Layout {
        operations: &LAYOUT_OPS,
        self_: model.cast::<LayoutSelf>(),
    }
}

fn abstract_model(value: Abstract) -> &'static AbstractModel {
    assert!(!value.capability.is_null());
    // Only callbacks installed by AbstractBinding call this function.
    // repr(C) places its capability first, independently of the model's layout.
    unsafe { &(*value.capability.cast::<AbstractBinding>()).model }
}

fn layout_model(value: Layout) -> &'static LayoutModel {
    if value.self_.is_null() {
        std::process::abort();
    }
    unsafe { &*value.self_.cast::<LayoutModel>() }
}

pub fn same(left: Abstract, right: Abstract) -> bool {
    left.capability == right.capability
}

pub fn unknown() -> Abstract {
    unsafe { ttx_unknown() }
}

pub fn empty_layout() -> Layout {
    unsafe { ttx_empty_layout() }
}

pub fn bytes_requirement() -> Abstract {
    unsafe { ttx_bytes_requirement() }
}

#[repr(C)]
struct ForwardBytes {
    candidate: Abstract,
    result: BytesResult,
}
extern "C" fn forwarded_bytes_unknown(self_: BytesResult) {
    let frame = unsafe { &*self_.self_.cast::<ForwardBytes>() };
    unsafe { ((*frame.result.operations).unknown)(frame.result) }
}
extern "C" fn forwarded_bytes_none(self_: BytesResult) {
    let frame = unsafe { &*self_.self_.cast::<ForwardBytes>() };
    unsafe { ((*frame.result.operations).none)(frame.result) }
}
#[repr(C)]
struct ByteView {
    candidate: Abstract,
    source: Bytes,
}
extern "C" fn byte_candidate(view: Bytes) -> Abstract {
    unsafe { (*view.self_.cast::<ByteView>()).candidate }
}
extern "C" fn byte_size(view: Bytes) -> u64 {
    let source = unsafe { (*view.self_.cast::<ByteView>()).source };
    unsafe { ((*source.operations).size)(source) }
}
extern "C" fn byte_visit(view: Bytes, sink: BytesSink) {
    let source = unsafe { (*view.self_.cast::<ByteView>()).source };
    unsafe { ((*source.operations).visit)(source, sink) }
}
extern "C" fn forwarded_bytes_resolved(self_: BytesResult, source: Bytes) {
    let frame = unsafe { &*self_.self_.cast::<ForwardBytes>() };
    let view = ByteView {
        candidate: frame.candidate,
        source,
    };
    let ops = BytesOps {
        header: AbiHeader {
            size: std::mem::size_of::<BytesOps>() as u32,
            abi_major: ABI_MAJOR,
            abi_minor: ABI_MINOR,
        },
        candidate: byte_candidate,
        size: byte_size,
        visit: byte_visit,
    };
    unsafe {
        ((*frame.result.operations).resolved)(
            frame.result,
            Bytes {
                operations: &ops,
                self_: (&view as *const ByteView).cast_mut().cast(),
            },
        )
    }
}
pub fn forward_bytes(source: Abstract, candidate: Abstract, result: BytesResult) {
    let frame = ForwardBytes { candidate, result };
    let ops = BytesResultOps {
        header: AbiHeader {
            size: std::mem::size_of::<BytesResultOps>() as u32,
            abi_major: ABI_MAJOR,
            abi_minor: ABI_MINOR,
        },
        unknown: forwarded_bytes_unknown,
        none: forwarded_bytes_none,
        resolved: forwarded_bytes_resolved,
    };
    unsafe {
        (source.ops().resolve_bytes)(
            source,
            BytesResult {
                operations: &ops,
                self_: (&frame as *const ForwardBytes).cast_mut().cast(),
            },
        )
    }
}

pub fn borrowed(value: &'static [u8]) -> BorrowedBytes {
    BorrowedBytes {
        data: value.as_ptr(),
        size: value.len() as u64,
    }
}

fn route(value: BorrowedBytes) -> Option<&'static [u8]> {
    if value.size == 0 {
        return Some(&[]);
    }
    if value.size > usize::MAX as u64 || (value.size != 0 && value.data.is_null()) {
        return None;
    }
    Some(unsafe { std::slice::from_raw_parts(value.data, value.size as usize) })
}

#[repr(C)]
struct InterfaceFrame {
    capability: InterfaceCapability,
    requirement: Abstract,
    candidate: Abstract,
    model: InterfaceModel,
}

fn with_interface(
    requirement: Abstract,
    candidate: Abstract,
    model: InterfaceModel,
    result: InterfaceSink,
) {
    let operations = InterfaceOps {
        header: AbiHeader {
            size: std::mem::size_of::<InterfaceOps>() as u32,
            abi_major: ABI_MAJOR,
            abi_minor: ABI_MINOR,
        },
        requirement: interface_requirement,
        candidate: interface_candidate,
        negotiate: interface_negotiate,
        invoke: interface_invoke,
    };
    let frame = InterfaceFrame {
        capability: InterfaceCapability {
            operations: &operations,
        },
        requirement,
        candidate,
        model,
    };
    let value = Interface {
        capability: &frame.capability,
    };
    unsafe { ((*result.operations).answer)(result, value) }
}

fn interface_frame<'call>(value: Interface) -> &'call InterfaceFrame {
    assert!(!value.capability.is_null());
    unsafe { &*value.capability.cast::<InterfaceFrame>() }
}

extern "C" fn abstract_name(value: Abstract) -> BorrowedBytes {
    borrowed(abstract_model(value).name)
}

extern "C" fn abstract_documentation(value: Abstract) -> Documentation {
    Documentation {
        operations: &DOCUMENTATION_OPS,
        self_: value.capability.cast_mut().cast::<DocumentationSelf>(),
    }
}

extern "C" fn abstract_resolve(value: Abstract, result: AbstractSink) {
    unsafe { ((*result.operations).answer)(result, value) }
}

extern "C" fn abstract_resolve_concept(value: Abstract, name: BorrowedBytes, result: AbstractSink) {
    let Some(name) = route(name) else {
        unsafe { ((*result.operations).answer)(result, unknown()) }
        return;
    };
    let model = abstract_model(value);
    let answer = model
        .concepts
        .iter()
        .find_map(|(route, answer)| (*route == name).then_some(*answer))
        .unwrap_or_else(unknown);
    unsafe { ((*result.operations).answer)(result, answer) }
}

extern "C" fn abstract_visit_concepts(value: Abstract, result: ConceptSink) {
    let model = abstract_model(value);
    for (name, answer) in &model.concepts {
        unsafe { ((*result.operations).item)(result, borrowed(name), *answer) }
    }
    unsafe { ((*result.operations).completed)(result) }
}

extern "C" fn abstract_interface(
    candidate: Abstract,
    requirement: Abstract,
    result: InterfaceSink,
) {
    let model = abstract_model(candidate);
    let selected = if same(candidate, requirement) {
        InterfaceModel {
            relation: InterfaceRelation::Equivalent,
            invoke: None,
        }
    } else {
        (model.interface)(candidate, requirement)
    };
    with_interface(requirement, candidate, selected, result);
}

extern "C" fn abstract_resolve_domain(value: Abstract, result: DomainResult) {
    match (abstract_model(value).domain)(value) {
        DomainProjection::Unknown => unsafe { ((*result.operations).unknown)(result) },
        DomainProjection::None => unsafe { ((*result.operations).none)(result) },
        DomainProjection::SelfWith(layout) => unsafe {
            ((*result.operations).resolved)(result, value, layout)
        },
    }
}

extern "C" fn abstract_resolve_callable(_value: Abstract, result: CallableResult) {
    unsafe { ((*result.operations).none)(result) }
}

extern "C" fn abstract_resolve_route(_value: Abstract, result: RouteResult) {
    unsafe { ((*result.operations).none)(result) }
}

extern "C" fn abstract_resolve_finite_extent(_value: Abstract, result: ExtentResult) {
    unsafe { ((*result.operations).none)(result) }
}

extern "C" fn abstract_resolve_bytes(_value: Abstract, result: BytesResult) {
    (abstract_model(_value).bytes)(_value, result);
}

extern "C" fn documentation_size(value: Documentation) -> u64 {
    abstract_model(Abstract {
        capability: value.self_.cast::<AbstractCapability>(),
    })
    .documentation
    .len() as u64
}

extern "C" fn documentation_visit(value: Documentation, result: BytesSink) {
    let model = abstract_model(Abstract {
        capability: value.self_.cast::<AbstractCapability>(),
    });
    if !model.documentation.is_empty() {
        unsafe { ((*result.operations).bytes)(result, borrowed(model.documentation)) }
    }
    unsafe { ((*result.operations).completed)(result) }
}

extern "C" fn interface_requirement(value: Interface) -> Abstract {
    interface_frame(value).requirement
}

extern "C" fn interface_candidate(value: Interface) -> Abstract {
    interface_frame(value).candidate
}

extern "C" fn interface_negotiate(value: Interface) -> InterfaceRelation {
    interface_frame(value).model.relation
}

extern "C" fn interface_invoke(
    value: Interface,
    operation: Abstract,
    input: Pack,
    context: Context,
    result: PackResult,
) {
    let frame = interface_frame(value);
    if frame.model.relation != InterfaceRelation::Satisfied
        && frame.model.relation != InterfaceRelation::Equivalent
    {
        unsafe { ((*result.operations).none)(result) }
        return;
    }
    if let Some(invoke) = &frame.model.invoke {
        invoke(operation, input, context, result);
    } else {
        unsafe { ((*result.operations).none)(result) }
    }
}

#[repr(C)]
struct EnumerableCapture {
    operations: EnumerableResultOps,
    answered: bool,
    enumerable: Option<Enumerable>,
}

fn enumerable_capture<'call>(value: EnumerableResult) -> &'call mut EnumerableCapture {
    if value.self_.is_null() {
        std::process::abort();
    }
    unsafe { &mut *value.self_.cast::<EnumerableCapture>() }
}

fn query_enumerable(layout: Layout) -> Option<Enumerable> {
    let mut capture = EnumerableCapture {
        operations: EnumerableResultOps {
            header: AbiHeader {
                size: std::mem::size_of::<EnumerableResultOps>() as u32,
                abi_major: ABI_MAJOR,
                abi_minor: ABI_MINOR,
            },
            rejected: enumerable_rejected,
            satisfied: enumerable_satisfied,
        },
        answered: false,
        enumerable: None,
    };
    let result = EnumerableResult {
        operations: &capture.operations,
        self_: (&mut capture as *mut EnumerableCapture).cast::<EnumerableResultSelf>(),
    };
    unsafe { ((*layout.operations).enumerable)(layout, result) }
    capture.answered.then_some(capture.enumerable).flatten()
}

extern "C" fn enumerable_rejected(value: EnumerableResult) {
    let capture = enumerable_capture(value);
    capture.answered = true;
    capture.enumerable = None;
}

extern "C" fn enumerable_satisfied(value: EnumerableResult, enumerable: Enumerable) {
    let capture = enumerable_capture(value);
    capture.answered = true;
    capture.enumerable = Some(enumerable);
}

#[derive(Copy, Clone, Eq, PartialEq)]
enum AdmissionStatus {
    Exact,
    Unknown,
    Rejected,
}

struct AdmissionFrame {
    entry: AdmissionEntryBinding,
    domain: AdmissionDomainBinding,
    expected: Vec<Abstract>,
    producers: Vec<Abstract>,
    status: AdmissionStatus,
    completed: bool,
}

#[repr(C)]
struct AdmissionEntryBinding {
    operations: LayoutEntrySinkOps,
    frame: *mut AdmissionFrame,
}

#[repr(C)]
struct AdmissionDomainBinding {
    operations: DomainResultOps,
    frame: *mut AdmissionFrame,
}

fn admission_from_entry<'call>(value: LayoutEntrySink) -> &'call mut AdmissionFrame {
    if value.self_.is_null() {
        std::process::abort();
    }
    let binding = unsafe { &*value.self_.cast::<AdmissionEntryBinding>() };
    if binding.frame.is_null() {
        std::process::abort();
    }
    unsafe { &mut *binding.frame }
}

fn admission_from_domain<'call>(value: DomainResult) -> &'call mut AdmissionFrame {
    if value.self_.is_null() {
        std::process::abort();
    }
    let binding = unsafe { &*value.self_.cast::<AdmissionDomainBinding>() };
    if binding.frame.is_null() {
        std::process::abort();
    }
    unsafe { &mut *binding.frame }
}

extern "C" fn admitted_domain_unknown(value: DomainResult) {
    apply_synthetic(value, None);
}

extern "C" fn admitted_domain_none(value: DomainResult) {
    apply_synthetic(value, None);
}

extern "C" fn admitted_domain_resolved(value: DomainResult, domain: Abstract, layout: Layout) {
    if layout.operations.is_null() {
        admission_from_domain(value).status = AdmissionStatus::Rejected;
        return;
    }
    apply_synthetic(value, Some(domain));
}

fn apply_synthetic(value: DomainResult, resolved: Option<Abstract>) {
    let (producer, expected) = {
        let admission = admission_from_domain(value);
        let index = admission.producers.len() - 1;
        if index >= admission.expected.len() {
            std::process::abort();
        }
        (admission.producers[index], admission.expected[index])
    };
    if resolved.is_some_and(|domain| same(domain, expected)) {
        return;
    }
    let synthetic = relation(producer, expected);
    let admission = admission_from_domain(value);
    if synthetic == InterfaceRelation::Rejected {
        admission.status = AdmissionStatus::Rejected;
    } else if synthetic == InterfaceRelation::Unknown
        && admission.status != AdmissionStatus::Rejected
    {
        admission.status = AdmissionStatus::Unknown;
    }
}

extern "C" fn admitted_entry(value: LayoutEntrySink, _path: BorrowedBytes, producer: Abstract) {
    let admission = admission_from_entry(value);
    admission.producers.push(producer);
    let accepted = admission.producers.len() <= admission.expected.len();
    if !accepted {
        admission.status = AdmissionStatus::Rejected;
        return;
    }
    let result = DomainResult {
        operations: &admission.domain.operations,
        self_: (&mut admission.domain as *mut AdmissionDomainBinding).cast::<DomainResultSelf>(),
    };
    unsafe { (producer.ops().resolve_domain)(producer, result) }
}

extern "C" fn admitted_completed(value: LayoutEntrySink) {
    admission_from_entry(value).completed = true;
}

pub enum Admission {
    SupportFailed,
    Unknown,
    Rejected,
    Exact {
        source_layout: Layout,
        producers: Vec<Abstract>,
    },
}

pub fn admit(input: Pack, expected: &[Abstract]) -> Admission {
    let source_layout = unsafe { ((*input.operations).layout)(input) };
    let Some(enumerable) = query_enumerable(source_layout) else {
        return Admission::Unknown;
    };
    if unsafe { ((*enumerable.operations).cardinality)(enumerable) } != expected.len() as u64 {
        return Admission::Rejected;
    }
    let mut admission = AdmissionFrame {
        entry: AdmissionEntryBinding {
            operations: LayoutEntrySinkOps {
                header: AbiHeader {
                    size: std::mem::size_of::<LayoutEntrySinkOps>() as u32,
                    abi_major: ABI_MAJOR,
                    abi_minor: ABI_MINOR,
                },
                entry: admitted_entry,
                completed: admitted_completed,
            },
            frame: std::ptr::null_mut(),
        },
        domain: AdmissionDomainBinding {
            operations: DomainResultOps {
                header: AbiHeader {
                    size: std::mem::size_of::<DomainResultOps>() as u32,
                    abi_major: ABI_MAJOR,
                    abi_minor: ABI_MINOR,
                },
                unknown: admitted_domain_unknown,
                none: admitted_domain_none,
                resolved: admitted_domain_resolved,
            },
            frame: std::ptr::null_mut(),
        },
        expected: expected.to_vec(),
        producers: Vec::with_capacity(expected.len()),
        status: AdmissionStatus::Exact,
        completed: false,
    };
    let frame = &mut admission as *mut AdmissionFrame;
    admission.entry.frame = frame;
    admission.domain.frame = frame;
    let visitor = LayoutEntrySink {
        operations: &admission.entry.operations,
        self_: (&mut admission.entry as *mut AdmissionEntryBinding).cast::<LayoutEntrySinkSelf>(),
    };
    unsafe { ((*enumerable.operations).visit)(enumerable, visitor) }
    if !admission.completed || admission.producers.len() != expected.len() {
        Admission::SupportFailed
    } else if admission.status == AdmissionStatus::Unknown {
        Admission::Unknown
    } else if admission.status == AdmissionStatus::Rejected {
        Admission::Rejected
    } else {
        Admission::Exact {
            source_layout,
            producers: admission.producers,
        }
    }
}

extern "C" fn layout_fit(value: Layout, input: Pack, context: Context, result: PackResult) {
    let model = layout_model(value);
    let Some(expected) = &model.receiving_domains else {
        unsafe { ((*result.operations).none)(result) }
        return;
    };
    match admit(input, expected) {
        Admission::SupportFailed => unsafe {
            ((*result.operations).support_failed)(result, PackSupportFailure::InvalidLayout)
        },
        Admission::Unknown => unsafe { ((*result.operations).unknown)(result) },
        Admission::Rejected => unsafe { ((*result.operations).none)(result) },
        Admission::Exact { source_layout, .. } => unsafe {
            ((*context.operations).pack)(context, source_layout, result)
        },
    }
}

extern "C" fn layout_enumerable(value: Layout, result: EnumerableResult) {
    let _ = layout_model(value);
    unsafe {
        ((*result.operations).satisfied)(
            result,
            Enumerable {
                operations: &ENUMERABLE_OPS,
                self_: value.self_.cast::<EnumerableSelf>(),
            },
        )
    }
}

extern "C" fn layout_named(_value: Layout, result: NamedResult) {
    unsafe { ((*result.operations).rejected)(result) }
}

#[repr(C)]
struct LayoutSnapshotState {
    operations: LayoutSnapshotOps,
    layout: Layout,
    _model: Box<LayoutModel>,
}

fn snapshot_state(value: LayoutSnapshot) -> *mut LayoutSnapshotState {
    value.self_.cast::<LayoutSnapshotState>()
}

extern "C" fn snapshot_layout(value: LayoutSnapshot) -> Layout {
    unsafe { (*snapshot_state(value)).layout }
}

extern "C" fn snapshot_release(value: LayoutSnapshot) {
    unsafe {
        drop(Box::from_raw(snapshot_state(value)));
    }
}

extern "C" fn layout_snapshot(value: Layout, result: LayoutSnapshotResult) {
    let mut model = Box::new(layout_model(value).clone());
    let retained_layout = Layout {
        operations: &LAYOUT_OPS,
        self_: (&mut *model as *mut LayoutModel).cast(),
    };
    let snapshot = Box::new(LayoutSnapshotState {
        operations: LayoutSnapshotOps {
            header: AbiHeader {
                size: std::mem::size_of::<LayoutSnapshotOps>() as u32,
                abi_major: ABI_MAJOR,
                abi_minor: ABI_MINOR,
            },
            layout: snapshot_layout,
            release: snapshot_release,
        },
        layout: retained_layout,
        _model: model,
    });
    let snapshot = Box::into_raw(snapshot);
    unsafe {
        ((*result.operations).retained)(
            result,
            LayoutSnapshot {
                operations: &(*snapshot).operations,
                self_: snapshot.cast::<LayoutSnapshotSelf>(),
            },
        )
    }
}

extern "C" fn layout_fluid(value: Layout, result: FluidResult) {
    unsafe {
        ((*result.operations).satisfied)(
            result,
            Fluid {
                operations: &FLUID_OPS,
                self_: value.self_.cast::<FluidSelf>(),
            },
        )
    }
}

extern "C" fn layout_value(_value: Layout, result: ValueLayoutResult) {
    unsafe { ((*result.operations).rejected)(result) }
}

extern "C" fn layout_composite(_value: Layout, result: CompositeLayoutResult) {
    unsafe { ((*result.operations).rejected)(result) }
}

extern "C" fn layout_ranged(_value: Layout, result: RangedLayoutResult) {
    unsafe { ((*result.operations).rejected)(result) }
}

extern "C" fn layout_reindexed(_value: Layout, result: ReindexedLayoutResult) {
    unsafe { ((*result.operations).rejected)(result) }
}

extern "C" fn fluid_layout(value: Fluid) -> Layout {
    Layout {
        operations: &LAYOUT_OPS,
        self_: value.self_.cast::<LayoutSelf>(),
    }
}

extern "C" fn enumerable_layout(value: Enumerable) -> Layout {
    Layout {
        operations: &LAYOUT_OPS,
        self_: value.self_.cast::<LayoutSelf>(),
    }
}

extern "C" fn enumerable_cardinality(value: Enumerable) -> u64 {
    layout_model(enumerable_layout(value)).entries.len() as u64
}

extern "C" fn enumerable_visit(value: Enumerable, result: LayoutEntrySink) {
    let model = layout_model(enumerable_layout(value));
    for (path, producer) in &model.entries {
        let path = BorrowedBytes {
            data: path.as_ptr(),
            size: path.len() as u64,
        };
        unsafe { ((*result.operations).entry)(result, path, *producer) }
    }
    unsafe { ((*result.operations).completed)(result) }
}

#[repr(C)]
struct RelationCapture {
    operations: InterfaceSinkOps,
    answered: bool,
    relation: InterfaceRelation,
    candidate: Abstract,
    requirement: Abstract,
}

fn relation_capture<'call>(value: InterfaceSink) -> &'call mut RelationCapture {
    if value.self_.is_null() {
        std::process::abort();
    }
    unsafe { &mut *value.self_.cast::<RelationCapture>() }
}

extern "C" fn relation_answer(value: InterfaceSink, interface: Interface) {
    let capture = relation_capture(value);
    capture.answered = true;
    capture.relation = unsafe { (interface.ops().negotiate)(interface) };
    capture.candidate = unsafe { (interface.ops().candidate)(interface) };
    capture.requirement = unsafe { (interface.ops().requirement)(interface) };
}

pub fn relation(candidate: Abstract, requirement: Abstract) -> InterfaceRelation {
    let mut capture = RelationCapture {
        operations: InterfaceSinkOps {
            header: AbiHeader {
                size: std::mem::size_of::<InterfaceSinkOps>() as u32,
                abi_major: ABI_MAJOR,
                abi_minor: ABI_MINOR,
            },
            answer: relation_answer,
        },
        answered: false,
        relation: InterfaceRelation::Unknown,
        candidate: unknown(),
        requirement: unknown(),
    };
    let result = InterfaceSink {
        operations: &capture.operations,
        self_: (&mut capture as *mut RelationCapture).cast::<InterfaceSinkSelf>(),
    };
    unsafe { (candidate.ops().interface)(candidate, requirement, result) }
    if !capture.answered
        || !same(capture.candidate, candidate)
        || !same(capture.requirement, requirement)
    {
        InterfaceRelation::Rejected
    } else {
        capture.relation
    }
}

#[repr(C)]
struct ForwardInvocation {
    operations: InterfaceSinkOps,
    candidate: Abstract,
    requirement: Abstract,
    operation: Abstract,
    input: Pack,
    context: Context,
    result: PackResult,
}

fn forward_invocation(value: InterfaceSink) -> &'static ForwardInvocation {
    if value.self_.is_null() {
        std::process::abort();
    }
    unsafe { &*value.self_.cast::<ForwardInvocation>() }
}

extern "C" fn forward_answer(value: InterfaceSink, interface: Interface) {
    let call = forward_invocation(value);
    let candidate = unsafe { (interface.ops().candidate)(interface) };
    let requirement = unsafe { (interface.ops().requirement)(interface) };
    let relation = unsafe { (interface.ops().negotiate)(interface) };
    if !same(candidate, call.candidate) || !same(requirement, call.requirement) {
        unsafe {
            ((*call.result.operations).support_failed)(
                call.result,
                PackSupportFailure::InvalidLayout,
            )
        }
    } else if relation == InterfaceRelation::Unknown {
        unsafe { ((*call.result.operations).unknown)(call.result) }
    } else if relation == InterfaceRelation::Rejected {
        unsafe { ((*call.result.operations).none)(call.result) }
    } else {
        unsafe {
            (interface.ops().invoke)(
                interface,
                call.operation,
                call.input,
                call.context,
                call.result,
            )
        }
    }
}

pub fn invoke(
    candidate: Abstract,
    requirement: Abstract,
    operation: Abstract,
    input: Pack,
    context: Context,
    result: PackResult,
) {
    let call = ForwardInvocation {
        operations: InterfaceSinkOps {
            header: AbiHeader {
                size: std::mem::size_of::<InterfaceSinkOps>() as u32,
                abi_major: ABI_MAJOR,
                abi_minor: ABI_MINOR,
            },
            answer: forward_answer,
        },
        candidate,
        requirement,
        operation,
        input,
        context,
        result,
    };
    let callback = InterfaceSink {
        operations: &call.operations,
        self_: (&call as *const ForwardInvocation)
            .cast_mut()
            .cast::<InterfaceSinkSelf>(),
    };
    unsafe { (candidate.ops().interface)(candidate, requirement, callback) }
}

unsafe extern "C" {
    fn ttx_unknown() -> Abstract;
    fn ttx_empty_layout() -> Layout;
    fn ttx_bytes_requirement() -> Abstract;
}

static ABSTRACT_OPS: AbstractOps = AbstractOps {
    header: AbiHeader {
        size: std::mem::size_of::<AbstractOps>() as u32,
        abi_major: ABI_MAJOR,
        abi_minor: ABI_MINOR,
    },
    name: abstract_name,
    documentation: abstract_documentation,
    resolve: abstract_resolve,
    resolve_concept: abstract_resolve_concept,
    visit_concepts: abstract_visit_concepts,
    interface: abstract_interface,
    resolve_domain: abstract_resolve_domain,
    resolve_callable: abstract_resolve_callable,
    resolve_route: abstract_resolve_route,
    resolve_finite_extent: abstract_resolve_finite_extent,
    resolve_bytes: abstract_resolve_bytes,
};

static DOCUMENTATION_OPS: DocumentationOps = DocumentationOps {
    header: AbiHeader {
        size: std::mem::size_of::<DocumentationOps>() as u32,
        abi_major: ABI_MAJOR,
        abi_minor: ABI_MINOR,
    },
    size: documentation_size,
    visit_bytes: documentation_visit,
};

static LAYOUT_OPS: LayoutOps = LayoutOps {
    header: AbiHeader {
        size: std::mem::size_of::<LayoutOps>() as u32,
        abi_major: ABI_MAJOR,
        abi_minor: ABI_MINOR,
    },
    fit: layout_fit,
    enumerable: layout_enumerable,
    named: layout_named,
    snapshot: layout_snapshot,
    fluid: layout_fluid,
    value: layout_value,
    composite: layout_composite,
    ranged: layout_ranged,
    reindexed: layout_reindexed,
};

static FLUID_OPS: FluidOps = FluidOps {
    header: AbiHeader {
        size: std::mem::size_of::<FluidOps>() as u32,
        abi_major: ABI_MAJOR,
        abi_minor: ABI_MINOR,
    },
    layout: fluid_layout,
};

static ENUMERABLE_OPS: EnumerableOps = EnumerableOps {
    header: AbiHeader {
        size: std::mem::size_of::<EnumerableOps>() as u32,
        abi_major: ABI_MAJOR,
        abi_minor: ABI_MINOR,
    },
    layout: enumerable_layout,
    cardinality: enumerable_cardinality,
    visit: enumerable_visit,
};
