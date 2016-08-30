#include "noldor_impl.h"

namespace noldor {

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
