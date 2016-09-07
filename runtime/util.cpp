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

#include "noldor_impl.h"

#include <sstream>

#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

namespace noldor {

noldor_exception::noldor_exception(std::string msg)
    : _msg(std::move(msg))
{}

const char* noldor_exception::what() const noexcept
{
    return _msg.c_str();
}

base_error::base_error(std::string msg, noldor::value irritants)
    : noldor_exception(msg + ", irritants: " + printable(irritants)), _irritants(irritants)
{}

value base_error::irritants() const noexcept
{
    return _irritants;
}

void list_insert(list_t *list, list_t *elem)
{
    elem->prev = list;
    elem->next = list->next;
    list->next = elem;
    elem->next->prev = elem;
}

void list_remove(list_t *elem)
{
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;
    elem->next = elem;
    elem->prev = elem;
}

template <class From, class To>
struct value_converter;

template <class T>
struct value_converter<T, T>
{
    static T convert(T t)
    {
        return t;
    }
};

template <>
struct value_converter<bool, value>
{
    static value convert(bool b)
    {
        return mk_bool(b);
    }
};

template <>
struct value_converter<int32_t, value>
{
    static value convert(int32_t i)
    {
        return mk_int(i);
    }
};

template <>
struct value_converter<value, int32_t>
{
    static int32_t convert(value val)
    {
        return to_int(val);
    }
};

template <>
struct value_converter<std::string, value>
{
    static value convert(std::string s)
    {
        return mk_string(std::move(s));
    }
};

template <>
struct value_converter<value, std::string>
{
    static std::string convert(value val)
    {
        return string_get(val);
    }
};

template <>
struct value_converter<char_t, value>
{
    static value convert(char_t c)
    {
        return mk_char(c.character);
    }
};

template <>
struct value_converter<double, value>
{
    static value convert(double d)
    {
        return mk_double(d);
    }
};

template <class...>
struct arg_converter;

template <>
struct arg_converter<>
{
    template <class... Accumulated>
    static auto get(std::tuple<Accumulated...> &&acc, value argl) -> std::tuple<Accumulated...>
    {
        check_type(is_null, argl, "unexpected extra arguments");
        return acc;
    }
};

template <class T>
struct arg_converter<T>
{
    template <class... Accumulated>
    static auto get(std::tuple<Accumulated...> &&acc, value argl)
    {
        check_type(is_null, cdr(argl), "unexpected extra arguments");
        return std::tuple_cat(std::move(acc), std::make_tuple(value_converter<value, T>::convert(car(argl))));
    }
};

template <>
struct arg_converter<dot_tag, value>
{
    template <class... Accumulated>
    static auto get(std::tuple<Accumulated...> &&acc, value argl)
    {
        return std::tuple_cat(std::move(acc), std::make_tuple(dot_tag{}, argl));
    }
};

template <class T, class U, class... Rest>
struct arg_converter<T, U, Rest...>
{
    template <class... Accumulated>
    static auto get(std::tuple<Accumulated...> &&acc, value argl)
    {
        check_type(is_pair, argl, "expected more arguments");
        return arg_converter<U, Rest...>::get(std::tuple_cat(std::move(acc), std::make_tuple(value_converter<value, T>::convert(car(argl)))),
                                              cdr(argl));
    }
};

template <class FunType, class... Args, size_t... I>
inline auto apply_tuple_impl(FunType fn, std::tuple<Args...> &&arg_tuple, std::index_sequence<I...>)
{
    return fn(std::get<I>(arg_tuple)...);
}

template <class FunType, class... Args>
inline auto apply_tuple(FunType fn, std::tuple<Args...> &&arg_tuple)
{
    return apply_tuple_impl(std::forward<FunType>(fn), std::move(arg_tuple), std::make_index_sequence<sizeof...(Args)>{});
}

#define MAKE_C_FUNC_DISPATCHER(LISP_NAME, C_NAME, C_RETURN, ...) \
    static value C_NAME##_dispatcher(value argl) { \
        static C_RETURN (*fnptr)(__VA_ARGS__) = &C_NAME; \
        return value_converter<C_RETURN, value>::convert(apply_tuple(fnptr, arg_converter<__VA_ARGS__>::get(std::make_tuple(), argl))); \
    }

X_NOLDOR_SHARED_PROCEDURES(MAKE_C_FUNC_DISPATCHER)
#undef MAKE_C_FUNC_DISPATCHER

void noldor_init(int argc, char **argv)
{
    static std::mutex mutex;
    static bool initialized = false;

    if (!initialized) {
        std::lock_guard<std::mutex> locker(mutex);

        if (!initialized) {
#define REGISTER_DISPATCHER(LISP_NAME, C_NAME, C_RETURN, ...) \
    environment_define(environment_global(), \
                       symbol(LISP_NAME), \
                       mk_primitive_procedure(#C_NAME, C_NAME##_dispatcher));
            X_NOLDOR_SHARED_PROCEDURES(REGISTER_DISPATCHER)
#undef REGISTER_DISPATCHER

            set_command_line(argc, argv);

            environment_define(interaction_environment(), SYMBOL_LITERAL(current-input-port-value),  mk_input_port(STDIN_FILENO));
            environment_define(interaction_environment(), SYMBOL_LITERAL(current-output-port-value), mk_input_port(STDOUT_FILENO));
            environment_define(interaction_environment(), SYMBOL_LITERAL(current-error-port-value),  mk_input_port(STDERR_FILENO));

            initialized = true;
        }
    }
}

bool is_tagged_list(value list, value tag)
{
    return is_pair(list) && eq(car(list), tag);
}

value assq(value obj, value list)
{
    while (is_pair(list)) {
        if (is_pair(car(list)) && eq(caar(list), obj))
            return car(list);

        list = cdr(list);
    }

    return mk_bool(false);
}

bool eqv(value obj1, value obj2)
{
    if (eq(obj1, obj2))
        return true;

    if (object_metaobject(obj1) != object_metaobject(obj2))
        return false;

    if (is_char(obj1))
        return char_get(obj1) == char_get(obj2);

    return false;
}

bool eq(value obj1, value obj2)
{
    return uint64_t(obj1) == uint64_t(obj2);
}

bool equal(value obj1, value obj2)
{
    if (eq(obj1, obj2))
        return true;

    if (object_metaobject(obj1) != object_metaobject(obj2))
        return false;

    if (is_char(obj1))
        return char_get(obj1) == char_get(obj2);

    if (is_string(obj1))
        return string_get(obj1) == string_get(obj1);

    if (is_vector(obj1))
        return vector_get(obj1) == vector_get(obj2);

    if (is_pair(obj1)) {
        while (is_pair(obj1) && is_pair(obj2)) {
            if (!equal(car(obj1), car(obj2)))
                return false;

            obj1 = cdr(obj1);
            obj2 = cdr(obj2);
        }

        return equal(obj1, obj2);
    }

    return false;
}

std::string printable(value val)
{
    if (is_int(val))
        return std::to_string(to_int(val));

    if (is_double(val))
        return std::to_string(to_double(val));

    if (is_symbol(val))
        return symbol_to_string(val);

    auto metaobject = object_metaobject(val);

    if (!metaobject || !metaobject->repr) {
        std::stringstream stream;

        if (!metaobject)
            stream << "<#unknown ";
        else
            stream << "<#unprintable ";

        stream << "object 0x" << std::hex << uint64_t(val) << ">";
        return stream.str();
    }

    return metaobject->repr(val);
}

std::ostream & operator << (std::ostream & os, value val)
{
    return os << printable(val);
}

} // namespace noldor
