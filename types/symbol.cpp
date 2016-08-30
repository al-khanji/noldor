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
