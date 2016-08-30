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

namespace noldor {

struct eof_object_t {};

static void eof_object_destruct(value self)
{
    object_data_as<eof_object_t *>(self)->~eof_object_t();
}

static void eof_object_gc_visit(value, gc_visit_fn_t, void*)
{}

static std::string eof_object_repr(value)
{
    return "<#eof-object>";
}

static metatype_t *eof_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_none,
        eof_object_destruct,
        eof_object_gc_visit,
        eof_object_repr
    };

    return &metaobject;
}

value mk_eof_object()
{
    return object_allocate<eof_object_t>(eof_metaobject(), {});
}

bool is_eof_object(value val)
{
    return object_metaobject(val) == eof_metaobject();
}

}
