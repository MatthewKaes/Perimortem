import lldb
import lldb.formatters.Logger

max_length = 64;

class ManagedTablePrinter:
  def __init__(self, valobj, dict):
    lldb.formatters.Logger._lldb_formatters_debug_level = 2
    self.valobj = valobj
    self.block = None
    self.size = None
    self.capacity = None
    self.elements_cache = []

  def update(self):
    logger = lldb.formatters.Logger.Logger()
    self.block = None
    self.size = None
    self.capacity = None
    try:
      self.arena = self.valobj.GetChildMemberWithName("arena")

      self.block = self.valobj.GetChildMemberWithName("rented_block")
      self.size = self.valobj.GetChildMemberWithName("size")
      self.capacity = self.valobj.GetChildMemberWithName("capacity")

      self.data_type = self.valobj.GetChildMemberWithName("rented_block").GetType().GetPointeeType()
      self.data_size = self.data_type.GetByteSize()

    except Exception as e:
      logger >> "Caught exception: %r" % e
      pass

  def num_children(self):
    return int(self.size.GetValueAsUnsigned())

  def has_children(self):
    return True

  def get_child_index(self, name):
    try:
      return int(name.lstrip("[").rstrip("]"))
    except:
      return -1

  def get_child_at_index(self, index):
    logger = lldb.formatters.Logger.Logger()
    if index < 0:
      return None
    if index >= self.num_children():
      return None

    try:
      offset = index * self.data_size
      value = self.block.CreateChildAtOffset(
                "[" + str(index) + "]", offset, self.data_type)
      name = value.GetChildMemberWithName("name")
      data = value.GetChildMemberWithName("data")
      return self.valobj.CreateValueFromData("[%d] %s" % (index, name.GetSummary()), data.GetData(), data.GetType())
    except Exception as e:
      logger >> "Caught exception: %r" % e
      return None

def look_view_summary(valobj, dictionary):
    size = valobj.GetNonSyntheticValue().GetChildMemberWithName("size").GetValueAsUnsigned()
    return "Size: %d" % size

def string_view_summary(valueObject, dictionary):
    value = valueObject.GetChildMemberWithName('_M_str').GetSummary()
    return value.lstrip('"').rstrip('"')

def byte_view_summary(valueObject, dictionary):
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
  debugger.HandleCommand('type synthetic add -w data_types -l data_types.ManagedTablePrinter -x "Perimortem::Memory::Managed::Table<.+>$"')

  debugger.HandleCommand('type summary add -w data_types --python-function data_types.look_view_summary -x "Perimortem::Memory::Managed::Table<.+>$"')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.string_view_summary std::string_view')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.byte_view_summary Perimortem::Core::View::Amorphous')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.rpc_header_summary Perimortem::Storage::Json::JsonRpc')
  debugger.HandleCommand('type category enable data_types')