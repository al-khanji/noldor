#include "noldor.h"
#include <sstream>

namespace noldor {

struct char_t {
    uint32_t character;
};

static void char_destruct(value val)
{
    object_data_as<char_t *>(val)->~char_t();
}

static void char_gc_visit(value, gc_visit_fn_t, void*)
{}

static std::string char_repr(value self)
{
    auto c = object_data_as<char_t *>(self)->character;

    std::stringstream os;

    os << "#\\";

    switch (c) {
    case '\a':
        os << "alarm";
        break;
    case '\b':
        os << "backspace";
        break;
    case '\x7f':
        os << "delete";
        break;
    case '\x1b':
        os << "escape";
        break;
    case '\n':
        os << "newline";
        break;
    case '\0':
        os << "null";
        break;
    case '\r':
        os << "return";
        break;
    case ' ':
        os << "space";
        break;
    case '\t':
        os << "tab";
        break;
    default:
        if (isascii(int(c)))
            os << char(c);
        else
            os << std::hex << "0x" << c;
        break;
    }

    return os.str();
}

static metatype_t *char_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_self_eval,
        char_destruct,
        char_gc_visit,
        char_repr,
    };

    return &metaobject;
}

value mk_char(uint32_t c)
{
    value obj = allocate(char_metaobject(), sizeof(char_t), alignof(char_t));
    new (object_data(obj)) char_t { c };
    return obj;
}

bool is_char(value v)
{
    return object_metaobject(v) == char_metaobject();
}

}
