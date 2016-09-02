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
#include <vector>
#include <numeric>

namespace noldor {

struct vector_t {
    std::vector<value> elements;
};

static void vector_destruct(value obj)
{
    object_data_as<vector_t *>(obj)->~vector_t();
}

static void vector_gc_visit(value self, gc_visit_fn_t visitor, void *data)
{
    auto vec = object_data_as<vector_t *>(self);
    for (value &e : vec->elements)
        visitor(&e, data);
}

static std::string vector_repr(value obj)
{
    auto vec = object_data_as<vector_t *>(obj);
    auto res = std::accumulate(vec->elements.begin(), vec->elements.end(), std::string{}, [] (const std::string &acc, value v) {
        return acc + " " + printable(v);
    });

    res[0] = '(';
    return "#" + res + ")";
}

static metatype_t *vector_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_self_eval,
        vector_destruct,
        vector_gc_visit,
        vector_repr
    };

    return &metaobject;
}

value mk_vector(std::vector<value> elements)
{
    return object_allocate<vector_t>(vector_metaobject(), { std::move(elements) });
}

bool is_vector(value v)
{
    return object_metaobject(v) == vector_metaobject();
}

std::vector<value> vector_get(value vec)
{
    return object_data_as<vector_t *>(vec)->elements;
}

}
