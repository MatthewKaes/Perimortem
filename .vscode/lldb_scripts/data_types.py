import lldb
import lldb.formatters.Logger
import re

max_length = 64;

class VectorPrinter:
  def __init__(self, valobj, dict):
    lldb.formatters.Logger._lldb_formatters_debug_level = 2
    self.valobj = valobj
    self.block = None
    self.size = None
    self.elements_cache = []

  def update(self):
    logger = lldb.formatters.Logger.Logger()
    self.block = None
    self.size = None
    try:

      self.block = self.valobj.GetChildMemberWithName("source_block")
      ints = list(map(int, re.findall(r'\d+', self.valobj.GetType().GetName())));
      if len(ints) > 0:
        self.size = ints[0]
        self.data_type = self.block.GetType().GetArrayElementType()
      else:
        self.size = self.valobj.GetChildMemberWithName("size").GetValueAsUnsigned();
        self.data_type = self.block.GetType().GetPointeeType()

      if self.size > 10000000:
        self.size = -1;

      self.data_size = self.data_type.GetByteSize()
      logger >> "peri_except: test %r" % self.data_type.GetName()

    except Exception as e:
      logger >> "peri_except: %r" % e
      pass

  def num_children(self):
    if self.size > 0:
      return 1 + int(self.size)
    else:
      return 1

  def has_children(self):
    return self.size > 0

  def get_child_index(self, name):
    try:
      return int(name.lstrip("[").rstrip("]"))
    except:
      return -1

  def get_child_at_index(self, index):
    logger = lldb.formatters.Logger.Logger()
    if index == 0:
      object = self.valobj.CreateValueFromExpression('size', '(int)%d' % self.size)
      return object
    if index >= self.size + 1:
      return None

    try:
      offset = (index - 1) * self.data_size
      value = self.block.CreateChildAtOffset(
                "[%d]" % (index - 1), offset, self.data_type)
      return self.valobj.CreateValueFromData("[%d]" % (index - 1), value.GetData(), value.GetType())
    except Exception as e:
      logger >> "peri_except: %r" % e
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

    value = valueObject.GetChildMemberWithName('source_block').GetValueAsUnsigned()
    if value == 0:
      return "... empty ..."

    error = lldb.SBError()
    process = lldb.debugger.GetSelectedTarget().GetProcess()
    content = process.ReadMemory(value, size, error)

    if error.Success():
      data = content.decode('utf-8', errors='replace')
      return data
    else:
      return "[ uninitialized ]"


def vector_view_summary(valueObject, dictionary):
    logger = lldb.formatters.Logger.Logger()
    try:
      block = valueObject.GetNonSyntheticValue().GetChildMemberWithName("source_block")
      name = block.GetType().GetPointeeType().GetName().removeprefix("Perimortem::")
      
      size = valueObject.GetChildAtIndex(0).GetValueAsSigned()
      if (size == -1):
        return "[ uninitialized ] " + name;
      
      return ("[ %d ] " % size) + name;
    except Exception as e:
      logger >> "peri_except: %r" % e
      return None

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
  
  debugger.HandleCommand('type synthetic add -w data_types -l data_types.VectorPrinter -x "Perimortem::Core::.+::Vector<.+>$"')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.vector_view_summary -x "Vector<.+>$"')
  
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.string_view_summary std::string_view')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.byte_view_summary Perimortem::Core::View::Bytes')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.byte_view_summary Perimortem::Core::Access::Bytes')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.byte_view_summary Perimortem::Memory::Dynamic::Bytes')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.byte_view_summary Perimortem::Memory::Managed::Bytes')
  debugger.HandleCommand('type summary add -w data_types --python-function data_types.rpc_header_summary Perimortem::Storage::Json::JsonRpc')
  debugger.HandleCommand('type category enable data_types')