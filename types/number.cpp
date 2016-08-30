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

    static value mul(value a, value b)
    {
        return mk_int(int32_t(to_int(a) * to_int(b)));
    }

    static value div(value a, value b)
    {
        return mk_int(int32_t(to_int(a) / to_int(b)));
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

    static value mul(value a, value b)
    {
        return mk_double(to_double(a) * to_double(b));
    }

    static value div(value a, value b)
    {
        return mk_double(to_double(a) / to_double(b));
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

    static value mul(value a, value b)
    {
        return mk_double(to_int(a) * to_double(b));
    }

    static value div(value a, value b)
    {
        return mk_double(to_int(a) / to_double(b));
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

    static value mul(value a, value b)
    {
        return mk_double(to_double(a) * to_int(b));
    }

    static value div(value a, value b)
    {
        return mk_double(to_double(a) / to_int(b));
    }
};

template <class LeftT, class RightT>
struct numeric_op<LeftT, RightT>
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
};

static inline OperandType numeric_operand_type(value v)
{
    if (is_int(v))
        return OperandType_Int;

    if (is_double(v))
        return OperandType_Double;

    return OperandType_Reflect;
}

#define DISPATCH_BINARY_NUMERIC_OP(OP, N_1, N_2)                               \
	auto left_type = numeric_operand_type(N_1);                                \
    auto right_type = numeric_operand_type(N_2);                               \
                                                                               \
    switch (left_type) {                                                       \
    case OperandType_Int:                                                      \
        switch (right_type) {                                                  \
        case OperandType_Int:                                                  \
            N_1 = numeric_op<int_operant, int_operant>::OP(N_1, N_2);          \
            continue;                                                          \
        case OperandType_Double:                                               \
            N_1 = numeric_op<int_operant, double_operant>::OP(N_1, N_2);       \
            continue;                                                          \
        case OperandType_Reflect:                                              \
            N_1 = numeric_op<int_operant, reflect_operant>::OP(N_1, N_2);      \
            continue;                                                          \
        }                                                                      \
        break;                                                                 \
                                                                               \
    case OperandType_Double:                                                   \
        switch (right_type) {                                                  \
        case OperandType_Int:                                                  \
            N_1 = numeric_op<double_operant, int_operant>::OP(N_1, N_2);       \
            continue;                                                          \
        case OperandType_Double:                                               \
            N_1 = numeric_op<double_operant, double_operant>::OP(N_1, N_2);    \
            continue;                                                          \
        case OperandType_Reflect:                                              \
            N_1 = numeric_op<double_operant, reflect_operant>::OP(N_1, N_2);   \
            continue;                                                          \
        }                                                                      \
        break;                                                                 \
                                                                               \
    case OperandType_Reflect:                                                  \
        switch (right_type) {                                                  \
        case OperandType_Int:                                                  \
            N_1 = numeric_op<reflect_operant, int_operant>::OP(N_1, N_2);      \
            continue;                                                          \
        case OperandType_Double:                                               \
            N_1 = numeric_op<reflect_operant, double_operant>::OP(N_1, N_2);   \
            continue;                                                          \
        case OperandType_Reflect:                                              \
            N_1 = numeric_op<reflect_operant, reflect_operant>::OP(N_1, N_2);  \
            continue;                                                          \
        }                                                                      \
        break;                                                                 \
    }                                                                          \

value add(value argl, dot_tag)
{
	value result = mk_int(0);

	while (!is_null(argl)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(add, result, next);
	}

	return result;
}

value sub(value argl, dot_tag)
{
	check_type(is_pair, argl, "sub: expected at least one argument");
	
	value result = car(argl);
	argl = cdr(argl);

	while (!is_null(result)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(sub, result, next);
	}

	return result;
}

value mul(value argl, dot_tag)
{
	value result = mk_int(1);

	while (!is_null(argl)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(mul, result, next);
	}

	return result;
}

value div(value argl, dot_tag)
{
	check_type(is_pair, argl, "div: expected at least one argument");
	
	value result = car(argl);
	argl = cdr(argl);

	while (!is_null(result)) {
		value next = car(argl);
		argl = cdr(argl);
		DISPATCH_BINARY_NUMERIC_OP(div, result, next);
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
