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
