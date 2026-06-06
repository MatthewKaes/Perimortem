import lldb
import lldb.formatters.Logger


def pixel_summary(valueObject, dictionary):
    logger = lldb.formatters.Logger.Logger()
    try:
        r = valueObject.GetChildMemberWithName("red").GetValueAsUnsigned()
        g = valueObject.GetChildMemberWithName("green").GetValueAsUnsigned()
        b = valueObject.GetChildMemberWithName("blue").GetValueAsUnsigned()
        a = valueObject.GetChildMemberWithName("alpha").GetValueAsUnsigned()

        return "r:%d g:%d b:%d a:%d" % (r, g, b, a)
    except Exception as e:
        logger >> "peri_except: %r" % e
        return None


def image_summary(valueObject, dictionary):
    logger = lldb.formatters.Logger.Logger()
    try:
        width = valueObject.GetChildMemberWithName("width").GetValueAsUnsigned()
        height = valueObject.GetChildMemberWithName("height").GetValueAsUnsigned()

        if width == 0 or height == 0:
            return "Empty Image"

        return "%d x %d Image" % (width, height)
    except Exception as e:
        logger >> "peri_except: %r" % e
        return None


def __lldb_init_module(debugger, dict):
    debugger.HandleCommand(
        'type summary add -w graphics_types --python-function '
        'graphics_types.pixel_summary "Perimortem::Graphics::Pixel"'
    )
    debugger.HandleCommand(
        'type summary add -w graphics_types --python-function '
        'graphics_types.image_summary "Perimortem::Graphics::Image"'
    )
    debugger.HandleCommand('type category enable graphics_types')
