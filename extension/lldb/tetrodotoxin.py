"""LLDB presentation for the C11 carriers emitted from TTX sources."""

import lldb


_SUMMARY_LIMIT = 32


def _raw(value):
    return value.GetNonSyntheticValue()


def _render_value(value):
    if not value or not value.IsValid():
        return "<unavailable>"

    summary = value.GetSummary()
    if summary:
        return summary

    scalar = value.GetValue()
    if scalar:
        return scalar

    count = value.GetNumChildren()
    if count == 0:
        return "{}"

    rendered = []
    visible = min(count, _SUMMARY_LIMIT)
    for index in range(visible):
        child = value.GetChildAtIndex(index)
        child_text = _render_value(child)
        name = child.GetName() if child and child.IsValid() else None
        if name and not name.startswith("["):
            rendered.append(f"{name} = {child_text}")
        else:
            rendered.append(child_text)

    if count > visible:
        rendered.append("…")
    return "{ " + ", ".join(rendered) + " }"


def option_summary(value, _internal_dictionary):
    raw = _raw(value)
    selected = raw.GetChildMemberWithName("set")
    niche = raw.GetType().GetCanonicalType().IsPointerType()
    if not selected.IsValid() and niche:
        if raw.GetValueAsUnsigned() == 0:
            return "empty"
        return "{ " + _render_value(raw.Dereference()) + " }"

    if not selected.IsValid() or selected.GetValueAsUnsigned() == 0:
        return "empty"

    payload = raw.GetChildMemberWithName("value")
    return "{ " + _render_value(payload) + " }"


class OptionSyntheticProvider:
    def __init__(self, value, _internal_dictionary):
        self.value = value
        self.payload = lldb.SBValue()
        self.selected = False
        self.update()

    def update(self):
        raw = _raw(self.value)
        state = raw.GetChildMemberWithName("set")
        niche = raw.GetType().GetCanonicalType().IsPointerType()
        if not state.IsValid() and niche:
            self.selected = raw.GetValueAsUnsigned() != 0
            self.payload = raw.Dereference() if self.selected else lldb.SBValue()
            return False

        self.selected = (
            state.IsValid() and state.GetValueAsUnsigned() != 0
        )
        self.payload = raw.GetChildMemberWithName("value")
        return False

    def has_children(self):
        return self.selected and self.payload.IsValid()

    def num_children(self):
        return 1 if self.has_children() else 0

    def get_child_index(self, name):
        return 0 if name == "value" and self.has_children() else -1

    def get_child_at_index(self, index):
        return self.payload if index == 0 and self.has_children() else None


def _contiguous_state(value):
    raw = _raw(value)
    data = raw.GetChildMemberWithName("data")
    if not data.IsValid():
        data = raw.GetChildAtIndex(0)
    size_value = raw.GetChildMemberWithName("size")
    if not size_value.IsValid():
        size_value = raw.GetChildAtIndex(1)
    size = size_value.GetValueAsUnsigned()
    element_type = data.GetType().GetPointeeType()
    element_size = (
        element_type.GetByteSize() if element_type.IsValid() else 0
    )
    return data.GetValueAsUnsigned(), size, element_type, element_size


def _contiguous_child(value, index, state):
    address, size, element_type, element_size = state
    if (
        address == 0
        or index < 0
        or index >= size
        or not element_type.IsValid()
        or element_size == 0
    ):
        return lldb.SBValue()

    return value.CreateValueFromAddress(
        f"[{index}]", address + index * element_size, element_type
    )


def _quote_bytes(value, state):
    address, size, _element_type, _element_size = state
    visible = min(size, _SUMMARY_LIMIT)
    error = lldb.SBError()
    data = value.GetProcess().ReadMemory(address, visible, error)
    if not error.Success():
        return "<unavailable>"

    escaped = []
    for byte in data:
        if byte == 0x09:
            escaped.append("\\t")
        elif byte == 0x0A:
            escaped.append("\\n")
        elif byte == 0x0D:
            escaped.append("\\r")
        elif byte == 0x22:
            escaped.append('\\"')
        elif byte == 0x5C:
            escaped.append("\\\\")
        elif 0x20 <= byte <= 0x7E:
            escaped.append(chr(byte))
        else:
            escaped.append(f"\\x{byte:02X}")

    if size > visible:
        escaped.append("…")
    return '"' + "".join(escaped) + '"'


def contiguous_summary(value, _internal_dictionary):
    state = _contiguous_state(value)
    type_name = _raw(value).GetType().GetName()
    dynamic_bytes = type_name in ("Bytes", "Dynamic::Bytes")
    if state[1] == 0:
        return 'Dynamic::Bytes("")' if dynamic_bytes else "empty"

    opening = type_name.find("[") if type_name else -1
    element_name = (
        type_name[opening + 1 : -1]
        if opening >= 0 and type_name.endswith("]")
        else state[2].GetCanonicalType().GetName()
    )
    if dynamic_bytes or element_name == "U8":
        quoted = _quote_bytes(value, state)
        return f"Dynamic::Bytes({quoted})" if dynamic_bytes else quoted

    return f"{state[1]} {element_name}"


class ContiguousSyntheticProvider:
    def __init__(self, value, _internal_dictionary):
        self.value = value
        self.state = (0, 0, lldb.SBType(), 0)
        self.update()

    def update(self):
        self.state = _contiguous_state(self.value)
        return False

    def has_children(self):
        address, size, element_type, element_size = self.state
        return (
            address != 0
            and size != 0
            and element_type.IsValid()
            and element_size != 0
        )

    def num_children(self):
        return self.state[1] if self.has_children() else 0

    def get_child_index(self, name):
        if not name.startswith("[") or not name.endswith("]"):
            return -1
        try:
            return int(name[1:-1])
        except ValueError:
            return -1

    def get_child_at_index(self, index):
        if not self.has_children():
            return None
        child = _contiguous_child(self.value, index, self.state)
        return child if child.IsValid() else None


def range_summary(value, _internal_dictionary):
    raw = _raw(value)
    start = _render_value(raw.GetChildMemberWithName("start"))
    end = _render_value(raw.GetChildMemberWithName("end"))
    return f"{start}...{end}"


def __lldb_init_module(debugger, _internal_dictionary):
    debugger.HandleCommand(
        "type summary add -w tetrodotoxin --python-function "
        "tetrodotoxin.option_summary -x '^Option\\[.*\\]$'"
    )
    debugger.HandleCommand(
        "type synthetic add -w tetrodotoxin --python-class "
        "tetrodotoxin.OptionSyntheticProvider -x '^Option\\[.*\\]$'"
    )
    debugger.HandleCommand(
        "type summary add -w tetrodotoxin --python-function "
        "tetrodotoxin.contiguous_summary "
        "-x '^((View|Access)\\[.*\\]|(Dynamic::)?Bytes)$'"
    )
    debugger.HandleCommand(
        "type synthetic add -w tetrodotoxin --python-class "
        "tetrodotoxin.ContiguousSyntheticProvider "
        "-x '^((View|Access)\\[.*\\]|(Dynamic::)?Bytes)$'"
    )
    debugger.HandleCommand(
        "type summary add -w tetrodotoxin --python-function "
        "tetrodotoxin.range_summary -x '^Range\\[.*\\]$'"
    )
    debugger.HandleCommand("type category enable tetrodotoxin")
