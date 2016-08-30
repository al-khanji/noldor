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
#include <unordered_map>

namespace noldor {

struct symbol_t {
    std::string name;
    std::size_t hash;
};

static void symbol_destruct(value obj)
{
    object_data_as<symbol_t *>(obj)->~symbol_t();
}

static void symbol_gc_visit(value, gc_visit_fn_t, void *)
{}

static std::string symbol_repr(value obj)
{
    return object_data_as<symbol_t *>(obj)->name;
}

static metatype_t *symbol_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_static,
        symbol_destruct,
        symbol_gc_visit,
        symbol_repr
    };

    return &metaobject;
}

static std::unordered_map<std::size_t, value> *interned_symbols()
{
    static std::unordered_map<std::size_t, value> table;
    return &table;
}

value symbol(std::string s)
{
    auto hash = std::hash<std::string>{}(s);
    auto interned = interned_symbols();

    auto it = interned->find(hash);

    if (it != interned->end())
        return it->second;

    auto symval = object_allocate<symbol_t>(symbol_metaobject(), symbol_t { std::move(s), hash });
    interned->emplace(hash, symval);

    return symval;
}

bool is_symbol(value v)
{
    return object_metaobject(v) == symbol_metaobject();
}

std::string symbol_to_string(value v)
{
    check_type(is_symbol, v, "symbol_to_string: expected symbol");
    return object_data_as<symbol_t *>(v)->name;
}

}
