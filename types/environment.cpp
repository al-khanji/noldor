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
#include <numeric>
#include <sstream>

namespace noldor {

struct environment_t {
    environment_t() = default;
    environment_t(value o) : outer(o) {}

    value outer = list();
    std::unordered_map<uint64_t, value> symtab, parameters;
};

static void environment_destruct(value self)
{
    object_data_as<environment_t *>(self)->~environment_t();
}

static void environment_gc_visit(value self, gc_visit_fn_t visitor, void *data)
{
    auto env = object_data_as<environment_t *>(self);

    for (auto &pair : env->symtab)
        visitor(&pair.second, data);

    visitor(&env->outer, data);
}

static std::string environment_repr(value val)
{
    std::stringstream stream;

    auto data = static_cast<environment_t *>(object_data(val));

    stream << "<#environment with symbols ("
           << std::accumulate(data->symtab.begin(), data->symtab.end(), std::string{}, [] (const std::string &acc, const auto &elem) {
                  std::stringstream stream;
                  stream << "(<" << value(elem.first) << " 0x" << std::hex << uint64_t(elem.first) << "> " << value(elem.second) << ")";
                  std::string repr = stream.str();

                  if (acc.empty()) return repr;
                  return acc + " " + repr;
              })
          << ") and outer " << data->outer << ">";

    return stream.str();
}

static metatype_t *environment_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_none,
        environment_destruct,
        environment_gc_visit,
        environment_repr
    };

    return &metaobject;
}

value environment_global()
{
    static value global = mk_empty_environment();
    return global;
}

value mk_environment(value outer)
{
    if (!is_environment(outer) && !is_null(outer))
        throw type_error("mk_environment: outer must be an environment or null", outer);

    if (is_null(outer))
        outer = environment_global();

    return object_allocate<environment_t>(environment_metaobject(), environment_t(outer));
}

value mk_empty_environment()
{
    return object_allocate<environment_t>(environment_metaobject(), {});
}

bool is_environment(value v)
{
    return object_metaobject(v) == environment_metaobject();
}

value environment_find(value env, value sym)
{
    check_type(is_environment, env, "environment_find: expected environment as first argument");
    check_type(is_symbol, sym, "environment_find: expected symbol as second argument");

    while (!is_null(env)) {
        auto data = static_cast<environment_t *>(object_data(env));

        if (data->symtab.find(sym) != data->symtab.end())
            return env;

        env = data->outer;
    }

    return mk_bool(false);
}

value environment_get(value env, value sym)
{
    check_type(is_environment, env, "environment_get: expected environment as first argument");
    check_type(is_symbol, sym, "environment_get: expected symbol as second argument");

    env = environment_find(env, sym);

    if (is_false(env))
        throw variable_error("undefined variable", sym);

    auto data = static_cast<environment_t *>(object_data(env));
    return data->symtab.at(sym);
}

value environment_set(value env, value sym, value val)
{
    check_type(is_environment, env, "environment_set: expected environment as first argument");
    check_type(is_symbol, sym, "environment_set: expected symbol as second argument");

    env = environment_find(env, sym);

    if (is_false(env))
        throw variable_error("undefined variable", sym);

    auto data = static_cast<environment_t *>(object_data(env));
    data->symtab.at(sym) = val;
    return val;
}

value environment_define(value env, value sym, value val)
{
    check_type(is_environment, env, "environment_define: expected environment as first argument");
    check_type(is_symbol, sym, "environment_define: expected symbol as second argument");

    auto data = static_cast<environment_t *>(object_data(env));
    auto res = data->symtab.emplace(sym, val);
    if (!res.second)
        data->symtab.at(sym) = val;
    return val;
}

value environment(value import_sets)
{
    value env = mk_empty_environment();

    while (!is_null(import_sets)) {
        (void) /* TODO: actual import */ car(import_sets);
        import_sets = cdr(import_sets);
    }

    return env;
}

value null_environment(value version)
{
    (void) version;
    return mk_environment();
}

value interaction_environment()
{
    static value env = mk_environment();
    static basic_scope scope;

    if (scope.variables.empty())
        scope.variables = { &env };

    return env;
}

value parametrize(value parameters, dot_tag, value body)
{
    return cons(parameters, body);
}

}
