import lldb
import lldb.formatters.Logger

class JsonPrinter:
  def __init__(self, valobj, dict):
    lldb.formatters.Logger._lldb_formatters_debug_level = 2
    self.valobj = valobj
    self.state = None
    self.value = None
    self.sub_type = None
    self.children = 0

  def update(self):
    logger = lldb.formatters.Logger.Logger()
    try:
      self.state = self.valobj.GetChildMemberWithName("state").GetValueAsUnsigned()
      self.value = self.valobj.GetChildMemberWithName("value")
      match self.state:
        case 0:
          self.children = 0
        case 5:
          self.sub_type = self.value.GetChildMemberWithName("object").GetNonSyntheticValue()

          self.block = self.sub_type.GetChildMemberWithName("rented_block")
          self.size = self.sub_type.GetChildMemberWithName("size")
          self.data_type = self.block.GetType().GetPointeeType()
          self.data_size = self.data_type.GetByteSize()

          self.children = self.size.GetValueAsUnsigned()
        case _:
          self.children = 1

    except Exception as e:
      logger >> "Caught exception: %r" % e
      pass

  def num_children(self):
    logger = lldb.formatters.Logger.Logger()
    logger >> "TESTING_OBJ %r" % self.children
    return self.children

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
      match self.state:
        case 1: # Raw
          data = self.value.GetChildMemberWithName("string")
          return self.valobj.CreateValueFromData("value", data.GetData(), data.GetType())
        case 2: # String
          data = self.value.GetChildMemberWithName("string")
          return self.valobj.CreateValueFromData("value", data.GetData(), data.GetType())
        case 3: # Number
          data = self.value.GetChildMemberWithName("number")
          return self.valobj.CreateValueFromData("value", data.GetData(), data.GetType())
        case 4: # Real
          data = self.value.GetChildMemberWithName("real")
          return self.valobj.CreateValueFromData("value", data.GetData(), data.GetType())
        case 5: # Object
          offset = index * self.data_size
          value = self.block.CreateChildAtOffset(
                    "[" + str(index) + "]", offset, self.data_type)
          name = value.GetChildMemberWithName("name")
          data = value.GetChildMemberWithName("data")
          return self.valobj.CreateValueFromData("%s" % (name.GetSummary()), data.GetData(), data.GetType())
        case 6: # Array
          data = self.value.GetChildMemberWithName("array")
          return self.valobj.CreateValueFromData("items", data.GetData(), data.GetType())
        case 7: # Boolean
          data = self.value.GetChildMemberWithName("boolean")
          return self.valobj.CreateValueFromData("value", data.GetData(), data.GetType())
    except Exception as e:
      logger >> "Caught exception: %r" % e
      return None


def json_view_summary(valobj, dictionary):
    logger = lldb.formatters.Logger.Logger()
    state = valobj.GetNonSyntheticValue().GetChildMemberWithName("state").GetValueAsUnsigned()
    match state:
      case 1: # Raw
        return "Raw"
      case 2: # String
        return "String"
      case 3: # Number
        return "Number"
      case 4: # Real
        return "Real"
      case 5: # Object
        size = valobj.GetNonSyntheticValue().GetChildMemberWithName("value").GetChildMemberWithName("object").GetChildMemberWithName("size").GetValueAsUnsigned()
        return "Object (%r members)" % size
      case 6: # Array
        return "Array"
      case 7: # Boolean
        return "Boolean"

def __lldb_init_module(debugger, dict):
  debugger.HandleCommand('type synthetic add -w json -l json.JsonPrinter -x "Perimortem::Storage::Json::Node$"')

  debugger.HandleCommand('type summary add -w json --python-function json.json_view_summary -x "Perimortem::Storage::Json::Node$"')
  debugger.HandleCommand('type category enable json')