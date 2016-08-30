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

struct bool_t {};

static void bool_destruct(value val)
{
    object_data_as<bool_t *>(val)->~bool_t();
}

static void bool_gc_visit(value, gc_visit_fn_t, void*)
{}

static std::string true_repr(value)
{
    return "#t";
}

static std::string false_repr(value)
{
    return "#f";
}

static metatype_t *true_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags(typeflags_self_eval | typeflags_static),
        bool_destruct,
        bool_gc_visit,
        true_repr
    };

    return &metaobject;
}

static metatype_t *false_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags(typeflags_self_eval | typeflags_static),
        bool_destruct,
        bool_gc_visit,
        false_repr
    };

    return &metaobject;
}

value mk_true()
{
    static value v = object_allocate<bool_t>(true_metaobject(), {});
    return v;
}

value mk_false()
{
    static value v = object_allocate<bool_t>(false_metaobject(), {});
    return v;
}

value mk_bool(bool b)
{
    return b ? mk_true() : mk_false();
}

bool is_true(value v)
{
    return uint64_t(v) == uint64_t(mk_true());
}

bool is_false(value v)
{
    return uint64_t(v) == uint64_t(mk_false());
}

bool is_bool(value v)
{
    return is_false(v) || is_true(v);
}

bool is_truthy(value v)
{
    return !is_false(v);
}

}



















