#include "noldor_impl.h"

#include <sstream>

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

void register_function(const char *name, std::function<value(value)> fn)
{
    environment_define(globals::global_environment(),
                       symbol(name),
                       mk_primitive_procedure(name, std::move(fn)));
}

void register_type(metatype_t *metaobject)
{
    globals::register_type(metaobject);
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
struct value_converter<int, value>
{
    static value convert(int i)
    {
        return mk_int(i);
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
struct arg_converter<value, dot_tag>
{
    template <class... Accumulated>
    static auto get(std::tuple<Accumulated...> &&acc, value argl)
    {
        return std::tuple_cat(std::move(acc), std::make_tuple(argl, dot_tag{}));
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
        try { \
            static C_RETURN (*fnptr)(__VA_ARGS__) = &C_NAME; \
            return value_converter<C_RETURN, value>::convert(apply_tuple(fnptr, arg_converter<__VA_ARGS__>::get(std::make_tuple(), argl))); \
        } catch (std::exception &e) { \
            throw noldor::runtime_error(std::string(LISP_NAME " internal fault: ") + e.what()); \
        } \
    }

X_NOLDOR_SHARED_PROCEDURES(MAKE_C_FUNC_DISPATCHER)
#undef MAKE_C_FUNC_DISPATCHER

void noldor_init()
{
    static std::mutex mutex;
    static bool initialized = false;

    if (!initialized) {
        std::lock_guard<std::mutex> locker(mutex);

        if (!initialized) {
#define REGISTER_DISPATCHER(LISP_NAME, C_NAME, C_RETURN, ...) \
    environment_define(globals::global_environment(), \
                       symbol(LISP_NAME), \
                       mk_primitive_procedure(#C_NAME, C_NAME##_dispatcher));
            X_NOLDOR_SHARED_PROCEDURES(REGISTER_DISPATCHER)
#undef REGISTER_DISPATCHER

            register_type(globals::false_metaobject());
            register_type(globals::true_metaobject());
            register_type(globals::null_metaobject());
            register_type(globals::pair_metaobject());
            register_type(globals::environment_metaobject());
            register_type(globals::primitive_function_metaobject());
            register_type(globals::compound_function_metaobject());
            register_type(globals::vector_metaobject());
            register_type(globals::string_metaobject());
            register_type(globals::char_metaobject());
            register_type(globals::eof_metaobject());

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

bool eq(value obj1, value obj2)
{
    return uint64_t(obj1) == uint64_t(obj2);
}

template <class...>
struct numeric_op;

enum OperandType {
    OperandType_Int,
    OperandType_Double,
    OperandType_Reflect
};

typedef std::integral_constant<OperandType, OperandType_Int>     int_operant;
typedef std::integral_constant<OperandType, OperandType_Double>  double_operant;
typedef std::integral_constant<OperandType, OperandType_Reflect> reflect_operant;

template <>
struct numeric_op<int_operant, int_operant>
{
    static value add(value a, value b)
    {
        return mk_int(int32_t(to_int(a) + to_int(b)));
    }

    static value sub(value a, value b)
    {
        return mk_int(int32_t(to_int(a) - to_int(b)));
    }
};

template <>
struct numeric_op<double_operant, double_operant>
{
    static value add(value a, value b)
    {
        return mk_double(to_double(a) + to_double(b));
    }

    static value sub(value a, value b)
    {
        return mk_double(to_double(a) - to_double(b));
    }
};

template <>
struct numeric_op<int_operant, double_operant>
{
    static value add(value a, value b)
    {
        return mk_double(to_int(a) + to_double(b));
    }

    static value sub(value a, value b)
    {
        return mk_double(to_int(a) - to_double(b));
    }
};

template <>
struct numeric_op<double_operant, int_operant>
{
    static value add(value a, value b)
    {
        return mk_double(to_double(a) + to_int(b));
    }

    static value sub(value a, value b)
    {
        return mk_double(to_double(a) - to_int(b));
    }
};

template <class T>
struct numeric_op<reflect_operant, T>
{
    static value add(value a, value b)
    {
        if (object_metaobject(a)->add)
            return object_metaobject(a)->add(a, b);

        throw noldor::type_error("cannot compute (+ " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value sub(value a, value b)
    {
        if (object_metaobject(a)->sub)
            return object_metaobject(a)->sub(a, b);

        throw noldor::type_error("cannot compute (- " + printable(a) + " " + printable(b) + " )", list(a, b));
    }
};

template <class T>
struct numeric_op<T, reflect_operant>
{
    static value add(value a, value b)
    {
        if (object_metaobject(b)->add)
            return object_metaobject(b)->add(b, a);

        throw noldor::type_error("cannot compute (+ " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value sub(value a, value b)
    {
        if (object_metaobject(b)->sub)
            return object_metaobject(b)->sub(b, a);

        throw noldor::type_error("cannot compute (- " + printable(a) + " " + printable(b) + " )", list(a, b));
    }
};

template <>
struct numeric_op<reflect_operant, reflect_operant>
{
    static value add(value a, value b)
    {
        if (object_metaobject(a)->add)
            return object_metaobject(a)->add(a, b);

        if (object_metaobject(b)->add)
            return object_metaobject(b)->add(b, a);

        throw noldor::type_error("cannot compute (+ " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value sub(value a, value b)
    {
        if (object_metaobject(a)->sub)
            return object_metaobject(a)->sub(a, b);

        if (object_metaobject(b)->sub)
            return object_metaobject(b)->sub(b, a);

        throw noldor::type_error("cannot compute (- " + printable(a) + " " + printable(b) + " )", list(a, b));
    }
};

static inline OperandType numeric_operand_type(value v)
{
    if (is_int(v))
        return OperandType_Int;

    if (is_double(v))
        return OperandType_Double;

    return OperandType_Reflect;
}

#define DEFINE_NUMERIC_FUNCTION(NAME, OP, HEADER)                                         \
value NAME(value argl, dot_tag)                                                           \
{                                                                                         \
    HEADER                                                                                \
                                                                                          \
    while (!is_null(argl)) {                                                              \
        value next = car(argl);                                                           \
        argl = cdr(argl);                                                                 \
                                                                                          \
        auto left_type = numeric_operand_type(result);                                    \
        auto right_type = numeric_operand_type(next);                                     \
                                                                                          \
        switch (left_type) {                                                              \
        case OperandType_Int:                                                             \
            switch (right_type) {                                                         \
            case OperandType_Int:                                                         \
                result = numeric_op<int_operant, int_operant>::OP(result, next);          \
                continue;                                                                 \
            case OperandType_Double:                                                      \
                result = numeric_op<int_operant, double_operant>::OP(result, next);       \
                continue;                                                                 \
            case OperandType_Reflect:                                                     \
                result = numeric_op<int_operant, reflect_operant>::OP(result, next);      \
                continue;                                                                 \
            }                                                                             \
            break;                                                                        \
                                                                                          \
        case OperandType_Double:                                                          \
            switch (right_type) {                                                         \
            case OperandType_Int:                                                         \
                result = numeric_op<double_operant, int_operant>::OP(result, next);       \
                continue;                                                                 \
            case OperandType_Double:                                                      \
                result = numeric_op<double_operant, double_operant>::OP(result, next);    \
                continue;                                                                 \
            case OperandType_Reflect:                                                     \
                result = numeric_op<double_operant, reflect_operant>::OP(result, next);   \
                continue;                                                                 \
            }                                                                             \
            break;                                                                        \
                                                                                          \
        case OperandType_Reflect:                                                         \
            switch (right_type) {                                                         \
            case OperandType_Int:                                                         \
                result = numeric_op<reflect_operant, int_operant>::OP(result, next);      \
                continue;                                                                 \
            case OperandType_Double:                                                      \
                result = numeric_op<reflect_operant, double_operant>::OP(result, next);   \
                continue;                                                                 \
            case OperandType_Reflect:                                                     \
                result = numeric_op<reflect_operant, reflect_operant>::OP(result, next);  \
                continue;                                                                 \
            }                                                                             \
            break;                                                                        \
        }                                                                                 \
    }                                                                                     \
                                                                                          \
    return result;                                                                        \
}                                                                                         \

DEFINE_NUMERIC_FUNCTION(sum, add, value result = mk_int(0);)
DEFINE_NUMERIC_FUNCTION(sub, sub, value result = car(argl); argl = cdr(argl);)

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
        std::string string;
        std::stringstream stream(string);

        if (!metaobject)
            stream << "<#unknown ";
        else
            stream << "<#unprintable ";

        stream << "object 0x" << std::hex << uint64_t(val) << ">";
        return stream.str();
    }

    return metaobject->repr(val);
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

std::ostream & operator << (std::ostream & os, value val)
{
    return os << printable(val);
}

} // namespace noldor
