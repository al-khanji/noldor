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
#include <functional>
#include <sstream>

namespace noldor {

struct primitive_procedure_t {
    std::string name;
    std::function<value(value)> fn;
};

static void primitive_procedure_destruct(value self)
{
    object_data_as<primitive_procedure_t *>(self)->~primitive_procedure_t();
}

static void primitive_procedure_gc_visit(value, gc_visit_fn_t, void*)
{}

static std::string primitive_procedure_repr(value val)
{
    auto data = object_data_as<primitive_procedure_t *>(val);

    std::string str;
    std::stringstream stream(str);

    stream << "<#primitive-function";
    if (!data->name.empty())
        stream << " C_" << data->name;

    stream << " 0x" << std::hex << data << ">";
    return stream.str();
}

static metatype_t *primitive_procedure_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_none,
        primitive_procedure_destruct,
        primitive_procedure_gc_visit,
        primitive_procedure_repr
    };

    return &metaobject;
}

value mk_primitive_procedure(std::string name, value (*fptr)(value))
{
    return object_allocate<primitive_procedure_t>(primitive_procedure_metaobject(), { std::move(name), fptr });
}

bool is_primitive_procedure(value val)
{
    return object_metaobject(val) == primitive_procedure_metaobject();
}

value apply_primitive_procedure(value self, value argl)
{
    check_type(is_primitive_procedure, self, "apply_primitive_procedure: expected primitive procedure");
    return object_data_as<primitive_procedure_t *>(self)->fn(argl);
}

struct compound_procedure_t {
    value environment;
    value parameters;
    value body;
};

void compound_function_destruct(value val)
{
    object_data_as<compound_procedure_t *>(val)->~compound_procedure_t();
}

void compound_function_gc_visit(value val, gc_visit_fn_t visitor, void *data)
{
    auto proc = object_data_as<compound_procedure_t *>(val);
    visitor(&proc->environment, data);
    visitor(&proc->parameters, data);
    visitor(&proc->body, data);
}

std::string compound_function_repr(value val)
{
    std::string str;
    std::stringstream stream(str);

    auto proc = object_data_as<compound_procedure_t *>(val);
    stream << cons(SYMBOL_LITERAL(lambda), cons(proc->parameters, proc->body));
    return stream.str();
}

static metatype_t *compound_function_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_none,
        compound_function_destruct,
        compound_function_gc_visit,
        compound_function_repr
    };

    return &metaobject;
}

value mk_procedure(value parameters, value body, value env)
{
    return object_allocate<compound_procedure_t>(compound_function_metaobject(), { env, parameters, body });
}

bool is_compound_procedure(value proc)
{
    return object_metaobject(proc) == compound_function_metaobject();
}

value procedure_parameters(value proc)
{
    check_type(is_compound_procedure, proc, "procedure_parameters: expected compound procedure");
    return object_data_as<compound_procedure_t *>(proc)->parameters;
}

value procedure_environment(value proc)
{
    check_type(is_compound_procedure, proc, "procedure_environment: expected compound procedure");
    return object_data_as<compound_procedure_t *>(proc)->environment;
}

value procedure_body(value proc)
{
    check_type(is_compound_procedure, proc, "procedure_body: expected compound procedure");
    return object_data_as<compound_procedure_t *>(proc)->body;
}

bool is_procedure(value val)
{
    return is_primitive_procedure(val) || is_compound_procedure(val);
}

}
