/*

Copyright (c) 2016 Louai Al-Khanji

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "noldor.h"
#include <sstream>

namespace noldor {

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
    return object_allocate<char_t>(char_metaobject(), char_t { c });
}

uint32_t char_get(value val)
{
    check_type(is_char, val, "expected character");
    return object_data_as<char_t *>(val)->character;
}

bool is_char(value v)
{
    return object_metaobject(v) == char_metaobject();
}

}
