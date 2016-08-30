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
