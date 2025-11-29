import lldb

max_length = 64;

class __m256iPrinter:
  def __init__(self, valobj, internal_dict):
    self.valobj = valobj

  def update(self):
    self.v0 = self.valobj.GetChildAtIndex(0).GetValueAsUnsigned()
    self.v1 = self.valobj.GetChildAtIndex(1).GetValueAsUnsigned()
    self.v2 = self.valobj.GetChildAtIndex(2).GetValueAsUnsigned()
    self.v3 = self.valobj.GetChildAtIndex(3).GetValueAsUnsigned()

  def num_children(self):
    return 2

  def get_summary(self):
    return "AVX2 __m256i"

  def get_child_index(self, name):
    return int(name.lstrip('[').rstrip(']'))

  def get_child_at_index(self, index):
    if index == 0:
      lane = ""
      for i in range(8):
        lane += f'{((self.v0 >> (8 * i)) & 0xFF):02x} '

      for i in range(7):
        lane += f'{((self.v1 >> (8 * i)) & 0xFF):02x} '
      lane += f'{((self.v1 >> (8 * 7)) & 0xFF):02x}'

      object = self.valobj.CreateValueFromExpression('lane 1', '(std::string_view)"' + lane + '"')
      object.SetFormat(lldb.eFormatUnicode8)
      self.valobj.CreateChildAtOffset
      return object
    elif index == 1:
      lane = ""
      for i in range(8):
        lane += f'{((self.v2 >> (8 * i)) & 0xFF):02x} '

      for i in range(7):
        lane += f'{((self.v3 >> (8 * i)) & 0xFF):02x} '
      lane += f'{((self.v3 >> (8 * 7)) & 0xFF):02x}'

      object = self.valobj.CreateValueFromExpression('lane 2', '(std::string_view)"' + lane + '"')
      return object
    else:
      return None


def string_view_summary(valueObject, dictionary):
    value = valueObject.GetChildMemberWithName('_M_str').GetSummary()
    return value.lstrip('"').rstrip('"')

def managed_string_summary(valueObject, dictionary):
    size = valueObject.GetChildMemberWithName('size').GetValueAsUnsigned()
    if (size == 0):
      return "... empty ..."

    value = valueObject.GetChildMemberWithName('rented_block').GetSummary()
    extra = f'... ({size - max_length} bytes)' if size > max_length else ""
    return value.lstrip('"').rstrip('"')[0:min(max_length, size)] + extra

def rpc_header_summary(valueObject, dictionary):
    size1 = valueObject.GetChildMemberWithName('json_rpc').GetChildMemberWithName('size').GetValueAsUnsigned()
    size2 = valueObject.GetChildMemberWithName('method').GetChildMemberWithName('size').GetValueAsUnsigned()
    if size1 == 0 or size2 == 0:
      return "[Not Loaded]"

    version = valueObject.GetChildMemberWithName('json_rpc').GetSummary()
    method = valueObject.GetChildMemberWithName('method').GetSummary()
    return f'{method} (v{version})'

def __lldb_init_module(debugger, dict):
  debugger.HandleCommand('type summary add -w avx2 --python-function avx2.string_view_summary std::string_view')
  debugger.HandleCommand('type summary add -w avx2 --python-function avx2.managed_string_summary Perimortem::Memory::ManagedString')
  debugger.HandleCommand('type summary add -w avx2 --python-function avx2.rpc_header_summary Perimortem::Storage::Json::RpcHeader')
  debugger.HandleCommand('type synthetic add -w avx2 -l avx2.__m256iPrinter __m256i')
  debugger.HandleCommand('type summary add --summary-string "AVX2 256bit Register" __m256i')
  debugger.HandleCommand('type category enable avx2')
  debugger.HandleCommand('type category disable VectorTypes')