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

namespace noldor {

template <class...>
struct unary_numeric_op;
template <class...>
struct binary_numeric_op;

enum OperandType {
    OperandType_Int,
    OperandType_Double,
    OperandType_Reflect
};

typedef std::integral_constant<OperandType, OperandType_Int>     int_operant;
typedef std::integral_constant<OperandType, OperandType_Double>  double_operant;
typedef std::integral_constant<OperandType, OperandType_Reflect> reflect_operant;

template <>
struct unary_numeric_op<int_operant> {
    static bool is_zero(value a)
    {
        return to_int(a) == 0;
    }
    static bool is_positive(value a)
    {
        return to_int(a) > 0;
    }
    static bool is_negative(value a)
    {
        return to_int(a) < 0;
    }
    static bool is_odd(value a)
    {
        return to_int(a) % 2 != 0;
    }
    static bool is_even(value a)
    {
        return to_int(a) % 2 == 0;
    }
};

template <>
struct unary_numeric_op<double_operant> {
    static bool is_zero(value a)
    {
        return to_double(a) == 0;
    }
    static bool is_positive(value a)
    {
        return to_double(a) > 0;
    }
    static bool is_negative(value a)
    {
        return to_double(a) < 0;
    }
    static bool is_odd(value a)
    {
        return int32_t(to_double(a)) % 2 != 0;
    }
    static bool is_even(value a)
    {
        return int32_t(to_double(a)) % 2 == 0;
    }
};

template <>
struct unary_numeric_op<reflect_operant> {
    static bool is_zero(value a)
    {
        throw noldor::type_error("unimplemented: (zero? " + printable(a) + ")", a);
    }
    static bool is_positive(value a)
    {
        throw noldor::type_error("unimplemented: (positive? " + printable(a) + ")", a);
    }
    static bool is_negative(value a)
    {
        throw noldor::type_error("unimplemented: (negative? " + printable(a) + ")", a);
    }
    static bool is_odd(value a)
    {
        throw noldor::type_error("unimplemented: (odd? " + printable(a) + ")", a);
    }
    static bool is_even(value a)
    {
        throw noldor::type_error("unimplemented: (even? " + printable(a) + ")", a);
    }
};

#define DEFINE_BINARY_NUMERIC_OPS(LEFT_OPERAND_TYPE, RIGHT_OPERAND_TYPE, LEFT_CAST, RIGHT_CAST, RESULT_CAST) \
    template <>                                                   \
    struct binary_numeric_op<LEFT_OPERAND_TYPE, RIGHT_OPERAND_TYPE>      \
    {                                                             \
        static value add(value a, value b)                        \
        {                                                         \
            return RESULT_CAST(LEFT_CAST(a) + RIGHT_CAST(b));     \
        }                                                         \
        static value sub(value a, value b)                        \
        {                                                         \
            return RESULT_CAST(LEFT_CAST(a) - RIGHT_CAST(b));     \
        }                                                         \
        static value mul(value a, value b)                        \
        {                                                         \
            return RESULT_CAST(LEFT_CAST(a) * RIGHT_CAST(b));     \
        }                                                         \
        static value div(value a, value b)                        \
        {                                                         \
            return RESULT_CAST(LEFT_CAST(a) / RIGHT_CAST(b));     \
        }                                                         \
        static bool equals(value a, value b)                      \
        {                                                         \
            return LEFT_CAST(a) == RIGHT_CAST(b);                 \
        }                                                         \
        static bool st(value a, value b)                          \
        {                                                         \
            return LEFT_CAST(a) < RIGHT_CAST(b);                  \
        }                                                         \
        static bool gt(value a, value b)                          \
        {                                                         \
            return LEFT_CAST(a) > RIGHT_CAST(b);                  \
        }                                                         \
        static bool ste(value a, value b)                         \
        {                                                         \
            return LEFT_CAST(a) <= RIGHT_CAST(b);                 \
        }                                                         \
        static bool gte(value a, value b)                         \
        {                                                         \
            return LEFT_CAST(a) >= RIGHT_CAST(b);                 \
        }                                                         \
};

DEFINE_BINARY_NUMERIC_OPS(int_operant, int_operant, to_int, to_int, mk_int)
DEFINE_BINARY_NUMERIC_OPS(double_operant, double_operant, to_double, to_double, mk_double)
DEFINE_BINARY_NUMERIC_OPS(int_operant, double_operant, to_int, to_double, mk_double)
DEFINE_BINARY_NUMERIC_OPS(double_operant, int_operant, to_double, to_int, mk_double)

template <class LeftT, class RightT>
struct binary_numeric_op<LeftT, RightT>
{
    static value add(value a, value b)
    {
        throw noldor::type_error("cannot compute (+ " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value sub(value a, value b)
    {
        throw noldor::type_error("cannot compute (- " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value mul(value a, value b)
    {
        throw noldor::type_error("cannot compute (* " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value div(value a, value b)
    {
        throw noldor::type_error("cannot compute (/ " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value equals(value a, value b)
    {
        throw noldor::type_error("cannot compute (= " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value st(value a, value b)
    {
        throw noldor::type_error("cannot compute (< " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value gt(value a, value b)
    {
        throw noldor::type_error("cannot compute (> " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value ste(value a, value b)
    {
        throw noldor::type_error("cannot compute (<= " + printable(a) + " " + printable(b) + " )", list(a, b));
    }

    static value gte(value a, value b)
    {
        throw noldor::type_error("cannot compute (>= " + printable(a) + " " + printable(b) + " )", list(a, b));
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

#define DISPATCH_UNARY_NUMERIC_OP(OP, N)                                        \
    switch (numeric_operand_type(N)) {                                          \
    case OperandType_Int: return unary_numeric_op<int_operant>::OP(N);          \
    case OperandType_Double: return unary_numeric_op<double_operant>::OP(N);    \
    case OperandType_Reflect: return unary_numeric_op<reflect_operant>::OP(N);  \
    }

#define DISPATCH_BINARY_NUMERIC_OP(OP, RES, N_1, N_2)                          \
	auto left_type = numeric_operand_type(N_1);                                \
    auto right_type = numeric_operand_type(N_2);                               \
                                                                               \
    switch (left_type) {                                                       \
    case OperandType_Int:                                                      \
        switch (right_type) {                                                  \
        case OperandType_Int:                                                  \
            RES = binary_numeric_op<int_operant, int_operant>::OP(N_1, N_2);          \
            break;                                                          \
        case OperandType_Double:                                               \
            RES = binary_numeric_op<int_operant, double_operant>::OP(N_1, N_2);       \
            break;                                                          \
        case OperandType_Reflect:                                              \
            RES = binary_numeric_op<int_operant, reflect_operant>::OP(N_1, N_2);      \
            break;                                                          \
        }                                                                      \
        break;                                                                 \
                                                                               \
    case OperandType_Double:                                                   \
        switch (right_type) {                                                  \
        case OperandType_Int:                                                  \
            RES = binary_numeric_op<double_operant, int_operant>::OP(N_1, N_2);       \
            break;                                                          \
        case OperandType_Double:                                               \
            RES = binary_numeric_op<double_operant, double_operant>::OP(N_1, N_2);    \
            break;                                                          \
        case OperandType_Reflect:                                              \
            RES = binary_numeric_op<double_operant, reflect_operant>::OP(N_1, N_2);   \
            break;                                                          \
        }                                                                      \
        break;                                                                 \
                                                                               \
    case OperandType_Reflect:                                                  \
        switch (right_type) {                                                  \
        case OperandType_Int:                                                  \
            RES = binary_numeric_op<reflect_operant, int_operant>::OP(N_1, N_2);        \
            break;                                                                   \
        case OperandType_Double:                                                        \
            RES = binary_numeric_op<reflect_operant, double_operant>::OP(N_1, N_2);     \
            break;                                                                   \
        case OperandType_Reflect:                                                       \
            RES = binary_numeric_op<reflect_operant, reflect_operant>::OP(N_1, N_2);    \
            break;                                                          \
        }                                                                      \
        break;                                                                 \
    }                                                                          \

value add(dot_tag, value argl)
{
	value result = mk_int(0);

	while (!is_null(argl)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(add, result, result, next);
	}

	return result;
}

value sub(dot_tag, value argl)
{
	check_type(is_pair, argl, "sub: expected at least one argument");

	value result = car(argl);
	argl = cdr(argl);

    while (!is_null(argl)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(sub, result, result, next);
	}

	return result;
}

value mul(dot_tag, value argl)
{
	value result = mk_int(1);

	while (!is_null(argl)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(mul, result, result, next);
	}

	return result;
}

value div(dot_tag, value argl)
{
	check_type(is_pair, argl, "div: expected at least one argument");

	value result = car(argl);
	argl = cdr(argl);

    while (!is_null(argl)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(div, result, result, next);
	}

	return result;
}

bool num_eq(dot_tag, value numbers)
{
    if (is_null(numbers) || is_null(cdr(numbers)))
        return true;

    bool result = true;

    while (!is_null(cdr(numbers))) {
        DISPATCH_BINARY_NUMERIC_OP(equals, result, car(numbers), cadr(numbers));

        if (result == false)
            break;

        numbers = cdr(numbers);
    }

    return result;
}

bool num_st(dot_tag, value numbers)
{
    if (is_null(numbers) || is_null(cdr(numbers)))
        return true;

    bool result = true;

    while (!is_null(cdr(numbers))) {
        DISPATCH_BINARY_NUMERIC_OP(st, result, car(numbers), cadr(numbers));

        if (result == false)
            break;

        numbers = cdr(numbers);
    }

    return result;
}

bool num_gt(dot_tag, value numbers)
{
    if (is_null(numbers) || is_null(cdr(numbers)))
        return true;

    bool result = true;

    while (!is_null(cdr(numbers))) {
        DISPATCH_BINARY_NUMERIC_OP(gt, result, car(numbers), cadr(numbers));

        if (result == false)
            break;

        numbers = cdr(numbers);
    }

    return result;
}

bool num_ste(dot_tag, value numbers)
{
    if (is_null(numbers) || is_null(cdr(numbers)))
        return true;

    bool result = true;

    while (!is_null(cdr(numbers))) {
        DISPATCH_BINARY_NUMERIC_OP(ste, result, car(numbers), cadr(numbers));

        if (result == false)
            break;

        numbers = cdr(numbers);
    }

    return result;
}

bool num_gte(dot_tag, value numbers)
{
    if (is_null(numbers) || is_null(cdr(numbers)))
        return true;

    bool result = true;

    while (!is_null(cdr(numbers))) {
        DISPATCH_BINARY_NUMERIC_OP(gte, result, car(numbers), cadr(numbers));

        if (result == false)
            break;

        numbers = cdr(numbers);
    }

    return result;
}

bool is_zero(value n)
{
    DISPATCH_UNARY_NUMERIC_OP(is_zero, n);
    NOLDOR_UNREACHABLE();
}

bool is_positive(value n)
{
    DISPATCH_UNARY_NUMERIC_OP(is_positive, n);
    NOLDOR_UNREACHABLE();
}

bool is_negative(value n)
{
    DISPATCH_UNARY_NUMERIC_OP(is_negative, n);
    NOLDOR_UNREACHABLE();
}

bool is_odd(value n)
{
    DISPATCH_UNARY_NUMERIC_OP(is_odd, n);
    NOLDOR_UNREACHABLE();
}

bool is_even(value n)
{
    DISPATCH_UNARY_NUMERIC_OP(is_even, n);
    NOLDOR_UNREACHABLE();
}

value max(dot_tag, value numbers)
{
    check_type(is_pair, numbers, "expected at least one argument");

    value result = car(numbers);
    numbers = cdr(numbers);

    while (!is_null(numbers)) {
        value candidate = car(numbers);
        numbers = cdr(numbers);

        bool is_greater;
        DISPATCH_BINARY_NUMERIC_OP(gt, is_greater, candidate, result);

        if (is_greater)
            result = candidate;
    }

    return result;
}

value min(dot_tag, value numbers)
{
    check_type(is_pair, numbers, "expected at least one argument");

    value result = car(numbers);
    numbers = cdr(numbers);

    while (!is_null(numbers)) {
        value candidate = car(numbers);
        numbers = cdr(numbers);

        bool is_greater;
        DISPATCH_BINARY_NUMERIC_OP(st, is_greater, candidate, result);

        if (is_greater)
            result = candidate;
    }

    return result;
}

bool is_number(value v)
{
    return is_int(v) || is_double(v);
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

}
