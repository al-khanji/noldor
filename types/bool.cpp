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



















