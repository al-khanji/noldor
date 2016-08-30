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

}
