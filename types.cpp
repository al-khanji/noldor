#include "noldor_impl.h"

#include <sstream>
#include <numeric>
#include <atomic>

namespace noldor {

static value self_eval(value val, value env)
{
    (void) env;
    return val;
}

static std::string false_repr(value)
{
    return "#f";
}

static std::string true_repr(value)
{
    return "#t";
}

static std::string null_repr(value)
{
    return "()";
}

static void pair_gc_visit(value self, gc_visit_fn_t visitor, void *data)
{
    auto pair = object_data_as<pair_t *>(self);
    visitor(reinterpret_cast<value *>(&pair->car), data);
    visitor(reinterpret_cast<value *>(&pair->cdr), data);
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

static void environment_destruct(value self)
{
    auto env = object_data_as<environment_t *>(self);
    env->symtab.~unordered_map();
}

static void environment_gc_visit(value self, gc_visit_fn_t visitor, void *data)
{
    auto env = object_data_as<environment_t *>(self);

    for (auto &pair : env->symtab)
        visitor(reinterpret_cast<value *>(&pair.second), data);

    visitor(reinterpret_cast<value *>(&env->outer), data);
}

static std::string environment_repr(value val)
{
    std::string str;
    std::stringstream stream(str);

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

static void primitive_function_destruct(value self)
{
    auto prim = object_data_as<primitive_function_t *>(self);
    prim->fn.~function();
}

static std::string primitive_function_repr(value val)
{
    auto data = object_data_as<primitive_function_t *>(val);

    std::string str;
    std::stringstream stream(str);

    stream << "<#primitive-function";
    if (!data->name.empty())
        stream << " C_" << data->name;

    stream << " 0x" << std::hex << data << ">";
    return stream.str();
}

static value primitive_function_apply(value self, value argl)
{
    auto prim = static_cast<primitive_function_t *>(object_data(self));
    return prim->fn(argl);
}

static std::string primitive_macro_repr(value val)
{
    std::string str;
    std::stringstream stream(str);
    stream << "<#primitive-macro 0x" << std::hex << object_data(val) << ">";
    return stream.str();
}

static value primitive_macro_expand(value self, value argl)
{
    auto prim = static_cast<primitive_function_t *>(object_data(self));
    return prim->fn(argl);
}

static void compound_function_gc_visit(value self, gc_visit_fn_t visitor, void *data)
{
    auto proc = object_data_as<compound_procedure_t *>(self);
    visitor(reinterpret_cast<value *>(&proc->parameters), data);
    visitor(reinterpret_cast<value *>(&proc->body), data);
    visitor(reinterpret_cast<value *>(&proc->environment), data);
}

static std::string compound_function_repr(value val)
{
    std::string str;
    std::stringstream stream(str);

    auto proc = object_data_as<compound_procedure_t *>(val);

    static auto lambda = symbol("lambda");
    stream << cons(lambda, cons(proc->parameters, proc->body));
    return stream.str();
}

metatype_t *globals::false_metaobject()
{
     static metatype_t metaobject = {
         &metaobject,
         METATYPE_VERSION,
         "#f",
         "false?",
         nullptr,
         nullptr,
         false_repr,
         self_eval,
         nullptr,
         nullptr,
         nullptr,
         nullptr
     };

     return &metaobject;
}

metatype_t *globals::true_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "#t",
        "true?",
        nullptr,
        nullptr,
        true_repr,
        self_eval,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

metatype_t *globals::null_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "null",
        "null?",
        nullptr,
        nullptr,
        null_repr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

metatype_t *globals::pair_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "pair",
        "pair?",
        nullptr,
        pair_gc_visit,
        pair_repr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

metatype_t *globals::environment_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "environment",
        "environment?",
        environment_destruct,
        environment_gc_visit,
        environment_repr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

metatype_t *globals::primitive_function_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "primitive-function",
        "primitive-function?",
        primitive_function_destruct,
        nullptr,
        primitive_function_repr,
        nullptr,
        nullptr,
        primitive_function_apply,
        nullptr,
        nullptr
    };

    return &metaobject;
}

metatype_t *globals::primitive_macro_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "primitive-macro",
        "primitive-macro?",
        primitive_function_destruct,
        nullptr,
        primitive_macro_repr,
        nullptr,
        primitive_macro_expand,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

metatype_t *globals::compound_function_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "compound-function",
        "compound-function?",
        nullptr,
        compound_function_gc_visit,
        compound_function_repr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

static std::string eof_object_repr(value self)
{
    (void) self;
    return "<#eof-object>";
}

metatype_t *globals::eof_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "eof-object",
        "eof-object?",
        nullptr,
        nullptr,
        eof_object_repr,
        self_eval,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

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

metatype_t *globals::vector_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "vector",
        "vector?",
        vector_destruct,
        vector_gc_visit,
        vector_repr,
        self_eval,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

static void string_destruct(value obj)
{
    object_data_as<string_t *>(obj)->~string_t();
}

static std::string string_repr(value obj)
{
    return '"' + object_data_as<string_t *>(obj)->string + '"';
}

metatype_t *globals::string_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "string",
        "string?",
        string_destruct,
        nullptr,
        string_repr,
        self_eval,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

static std::string char_repr(value self)
{
    auto c = object_data_as<char_t *>(self)->character;

    std::stringstream os;

    os << "#\\";

    switch (c) {
    case '\a':
        os << "alarm";
        break;
    case '\b':
        os << "backspace";
        break;
    case '\x7f':
        os << "delete";
        break;
    case '\x1b':
        os << "escape";
        break;
    case '\n':
        os << "newline";
        break;
    case '\0':
        os << "null";
        break;
    case '\r':
        os << "return";
        break;
    case ' ':
        os << "space";
        break;
    case '\t':
        os << "tab";
        break;
    default:
        os << char(c);
        break;
    }

    return os.str();
}

metatype_t *globals::char_metaobject()
{
    static metatype_t metaobject = {
        &metaobject,
        METATYPE_VERSION,
        "char",
        "char?",
        nullptr,
        nullptr,
        char_repr,
        self_eval,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    return &metaobject;
}

std::unordered_set<metatype_t *> &globals::metaobjects()
{
    static std::unordered_set<metatype_t *> objs;
    return objs;
}

std::unordered_map<std::size_t, uint64_t> &globals::symbols()
{
    static std::unordered_map<std::size_t, uint64_t> syms;
    return syms;
}

void globals::register_type(metatype_t *metaobject)
{
    globals::metaobjects().insert(metaobject);

    if (metaobject->predicate_name != nullptr)
        register_function(metaobject->predicate_name,
                          [metaobject] (value obj) { return mk_bool(metaobject == object_metaobject(car(obj))); });
}

value globals::intern_symbol(const std::string &name)
{
    auto hash = std::hash<std::string>{}(name);
    auto iter = symbols().find(hash);

    if (iter != symbols().end())
        return iter->second;

    auto sym = magic::from_symbol(new symbol_t { name, hash });
    symbols()[hash] = sym;
    return sym;
}

value globals::global_environment()
{
    static value env = mk_empty_environment();
    return env;
}

value list(value argl, dot_tag)
{
    return argl;
}

value symbol(const std::string &name)
{
    return globals::intern_symbol(name);
}

bool is_symbol(value v)
{
    return magic::is_symbol(v);
}

std::string symbol_to_string(value v)
{
    return magic::get_symbol(v)->name;
}

value list()
{
    return magic::from_pointer(globals::null_metaobject());
}

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

bool is_number(value v)
{
    return is_int(v) || is_double(v);
}

bool is_null(value v)
{
    auto val_meta = object_metaobject(v);
    auto null_meta = globals::null_metaobject();
    return val_meta == null_meta;
}

value append(value x, value y)
{
    if (is_null(x))
        return y;

    return cons(car(x), append(cdr(x), y));
}

value cons(value a, value b)
{
    value cell = allocate(globals::pair_metaobject(), sizeof(pair_t), alignof(pair_t));
    new (object_data(cell)) pair_t { a, b };
    return cell;
}

bool is_pair(value v)
{
    return object_metaobject(v) == globals::pair_metaobject();
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

value mk_bool(bool b)
{
    return magic::from_pointer(b ? globals::true_metaobject() : globals::false_metaobject());
}

bool is_bool(value v)
{
    return is_false(v) || is_true(v);
}

bool is_false(value v)
{
    return object_metaobject(v) == globals::false_metaobject();
}

bool is_true(value v)
{
    return object_metaobject(v) == globals::true_metaobject();
}

bool is_truthy(value v)
{
    return is_false(v) == false;
}

value mk_double(double d)
{
    return magic::from_double(d);
}

bool is_double(value v)
{
    return magic::is_double(v);
}

double to_double(value v)
{
    return magic::get_double(v);
}

value mk_int(int32_t i)
{
    return magic::from_int32(i);
}

bool is_int(value v)
{
    return magic::is_int32(v);
}

int32_t to_int(value v)
{
    return magic::get_int32(v);
}

value mk_environment(value outer)
{
    if (!is_environment(outer) && !is_null(outer))
        throw type_error("mk_environment: outer must be an environment or null", outer);

    if (is_null(outer))
        outer = globals::global_environment();

    auto env = mk_empty_environment();
    static_cast<environment_t *>(object_data(env))->outer = outer;
    return env;
}

value mk_empty_environment()
{
    value obj = allocate(globals::environment_metaobject(), sizeof(environment_t), alignof(environment_t));

    auto env = new (object_data(obj)) environment_t;
    env->outer = list();

    return obj;
}

bool is_environment(value v)
{
    return object_metaobject(v) == globals::environment_metaobject();
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
        return env;

    auto data = static_cast<environment_t *>(object_data(env));
    return data->symtab.at(sym);
}

value environment_set(value env, value sym, value val)
{
    check_type(is_environment, env, "environment_set: expected environment as first argument");
    check_type(is_symbol, sym, "environment_set: expected symbol as second argument");

    env = environment_find(env, sym);

    if (is_false(env))
        return env;

    auto data = static_cast<environment_t *>(object_data(env));
    return data->symtab[sym] = val;
}

value environment_define(value env, value sym, value val)
{
    check_type(is_environment, env, "environment_define: expected environment as first argument");
    check_type(is_symbol, sym, "environment_define: expected symbol as second argument");

    auto data = static_cast<environment_t *>(object_data(env));
    return data->symtab[sym] = val;
}

value mk_primitive_procedure(std::string name, std::function<value(value)> fn)
{
    auto obj = allocate(globals::primitive_function_metaobject(), sizeof(primitive_function_t), alignof(primitive_function_t));
    new (object_data(obj)) primitive_function_t { std::move(name), std::move(fn) };
    return obj;
}

value mk_primitive_procedure(std::function<value(value)> fn)
{
    return mk_primitive_procedure("", std::move(fn));
}

value mk_primitive_macro(std::function<value(value)> mac)
{
    auto obj = allocate(globals::primitive_macro_metaobject(), sizeof(primitive_function_t), alignof(primitive_function_t));
    new (object_data(obj)) primitive_function_t { "", std::move(mac) };
    return obj;
}

bool is_macro(value v)
{
    return is_primitive_macro(v);
}

bool is_primitive_macro(value v)
{
    return object_metaobject(v) == globals::primitive_macro_metaobject();
}

value mk_vector(std::vector<value> elements)
{
    value obj = allocate(globals::vector_metaobject(), sizeof(vector_t), alignof(vector_t));
    new (object_data(obj)) vector_t { std::move(elements) };
    return obj;
}

bool is_vector(value v)
{
    return object_metaobject(v) == globals::vector_metaobject();
}

value mk_string(std::string s)
{
    value obj = allocate(globals::string_metaobject(), sizeof(string_t), alignof(string_t));
    new (object_data(obj)) string_t { std::move(s) };
    return obj;
}

bool is_string(value v)
{
    return object_metaobject(v) == globals::string_metaobject();
}

value mk_char(uint32_t c)
{
    value obj = allocate(globals::char_metaobject(), sizeof(char_t), alignof(char_t));
    new (object_data(obj)) char_t { c };
    return obj;
}

bool is_char(value v)
{
    return object_metaobject(v) == globals::char_metaobject();
}

value mk_eof_object()
{
    return magic::from_pointer(globals::eof_metaobject());
}

bool is_eof_object(value v)
{
    return object_metaobject(v) == globals::eof_metaobject();
}

} // namespace noldor
