#include "noldor.h"
#include <sstream>
#include <unordered_set>

namespace noldor {

// null type

struct null_t {};

static void null_destruct(value val)
{
    object_data_as<null_t *>(val)->~null_t();
}

static void null_gc_visit(value, gc_visit_fn_t, void *)
{}

static std::string null_repr(value)
{
    static std::string repr = "()";
    return repr;
}

metatype_t *null_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_static,
        null_destruct,
        null_gc_visit,
        null_repr
    };

    return &metaobject;
}

value null()
{
    static value v = object_allocate<null_t>(null_metaobject(), {});
    return v;
}

value list()
{
    return null();
}

bool is_null(value val)
{
    return uint64_t(val) == uint64_t(null());
}

// pair type

struct pair_t {
    value car;
    value cdr;
};

static void pair_destruct(value val)
{
    object_data_as<pair_t *>(val)->~pair_t();
}

static void pair_gc_visit(value val, gc_visit_fn_t visitor, void *data)
{
    auto pair = object_data_as<pair_t *>(val);
    visitor(&pair->car, data);
    visitor(&pair->cdr, data);
}

static std::string pair_repr(value val)
{
    std::string repr;
    std::stringstream stream(repr);

    while (!is_null(val)) {
        if (is_pair(val)) {
            stream << " " << car(val);
            val = cdr(val);
        } else {
            stream << " . " << val;
            val = list();
        }
    }

    stream << ")";

    repr = stream.str();
    repr[0] = '(';

    return repr;
}

static metatype_t *pair_metaobject() {
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_none,
        pair_destruct,
        pair_gc_visit,
        pair_repr
    };

    return &metaobject;
}

value cons(value a, value b)
{
    return object_allocate<pair_t>(pair_metaobject(), {a, b});
}

bool is_pair(value v)
{
    return object_metaobject(v) == pair_metaobject();
}

value car(value v)
{
    check_type(is_pair, v, "car: expected pair");
    return reinterpret_cast<pair_t *>(object_data(v))->car;
}

value cdr(value v)
{
    check_type(is_pair, v, "cdr: expected pair");
    return reinterpret_cast<pair_t *>(object_data(v))->cdr;
}

value set_car(value pair, value val)
{
    check_type(is_pair, pair, "set_car: expected pair");
    return reinterpret_cast<pair_t *>(object_data(pair))->car = val;
}

value set_cdr(value pair, value val)
{
    check_type(is_pair, pair, "set_cdr: expected pair");
    return reinterpret_cast<pair_t *>(object_data(pair))->cdr = val;
}

// list utilities

bool is_list(value v)
{
    std::unordered_set<uint64_t> encountered_nodes;

    while (is_pair(v)) {
        auto it = encountered_nodes.find(v);
        if (it != encountered_nodes.end())
            return false;

        encountered_nodes.insert(v);
        v = cdr(v);
    }

    return is_null(v);
}

value append(value x, value y)
{
    if (is_null(x))
        return y;

    return cons(car(x), append(cdr(x), y));
}

value list(value argl, dot_tag)
{
    return argl;
}


value caar(value obj)
{
    return car(car(obj));
}

value cadr(value obj)
{
    return car(cdr(obj));
}

value cdar(value obj)
{
    return cdr(car(obj));
}

value cddr(value obj)
{
    return cdr(cdr(obj));
}

value caaar(value obj)
{
    return car(caar(obj));
}

value caadr(value obj)
{
    return car(cadr(obj));
}

value cadar(value obj)
{
    return car(cdar(obj));
}

value caddr(value obj)
{
    return car(cddr(obj));
}

value cdaar(value obj)
{
    return cdr(caar(obj));
}

value cdadr(value obj)
{
    return cdr(cadr(obj));
}

value cddar(value obj)
{
    return cdr(cdar(obj));
}

value cdddr(value obj)
{
    return cdr(cddr(obj));
}

value caaaar(value obj)
{
    return car(caaar(obj));
}

value caaadr(value obj)
{
    return car(caadr(obj));
}

value caadar(value obj)
{
    return car(cadar(obj));
}

value caaddr(value obj)
{
    return car(caddr(obj));
}

value cadaar(value obj)
{
    return car(cdaar(obj));
}

value cadadr(value obj)
{
    return car(cdadr(obj));
}

value caddar(value obj)
{
    return car(cddar(obj));
}

value cadddr(value obj)
{
    return car(cdddr(obj));
}

value cdaaar(value obj)
{
    return cdr(caaar(obj));
}

value cdaadr(value obj)
{
    return cdr(caadr(obj));
}

value cdadar(value obj)
{
    return cdr(cadar(obj));
}

value cdaddr(value obj)
{
    return cdr(caddr(obj));
}

value cddaar(value obj)
{
    return cdr(cdaar(obj));
}

value cddadr(value obj)
{
    return cdr(cdadr(obj));
}

value cdddar(value obj)
{
    return cdr(cddar(obj));
}

value cddddr(value obj)
{
    return cdr(cdddr(obj));
}

}
