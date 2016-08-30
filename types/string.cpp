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
#include <string>

namespace noldor {

struct string_t {
    std::string string;
};

static void string_destruct(value obj)
{
    object_data_as<string_t *>(obj)->~string_t();
}

static void string_gc_visit(value, gc_visit_fn_t, void *)
{}

static std::string string_repr(value obj)
{
    return '"' + object_data_as<string_t *>(obj)->string + '"';
}

static metatype_t *string_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_self_eval,
        string_destruct,
        string_gc_visit,
        string_repr
    };

    return &metaobject;
}

value mk_string(std::string s)
{
    return object_allocate<string_t>(string_metaobject(), { std::move(s) } );
}

bool is_string(value v)
{
    return object_metaobject(v) == string_metaobject();
}

std::string string_get(value val)
{
    check_type(is_string, val, "expected string");
    return object_data_as<string_t *>(val)->string;
}

}
