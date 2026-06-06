import lldb
import lldb.formatters.Logger


def format_size(size):
    if size < 1024:
        return "%d Bytes" % size
    elif size < 1024 * 1024:
        return "%.1f KB" % (size / 1024)
    elif size < 1024 * 1024 * 1024:
        return "%.1f MB" % (size / (1024 * 1024))
    else:
        return "%.1f GB" % (size / (1024 * 1024 * 1024))


def file_summary(valueObject, dictionary):
    logger = lldb.formatters.Logger.Logger()
    try:
        read_time = valueObject.GetChildMemberWithName("read_time").GetValueAsUnsigned()
        modified_time = valueObject.GetChildMemberWithName("modified_time").GetValueAsUnsigned()

        if read_time == 0 and modified_time == 0:
            return "Unloaded"

        size = valueObject.GetChildMemberWithName("data").GetChildMemberWithName("size").GetValueAsUnsigned()

        if read_time >= modified_time:
            return "Sourced " + format_size(size)
        else:
            return "Modified " + format_size(size)
    except Exception as e:
        logger >> "peri_except: %r" % e
        return None


def __lldb_init_module(debugger, dict):
    debugger.HandleCommand(
        'type summary add -w system_types --python-function '
        'system_types.file_summary "Perimortem::System::File"'
    )
    debugger.HandleCommand('type category enable system_types')
