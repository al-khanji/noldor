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

#ifndef NOLDOR_ECEVAL_H
#define NOLDOR_ECEVAL_H

#include "noldor.h"

#include <iostream>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(_POSIX_VERSION)
# define NOLDOR_POSIX 1
# include <sys/types.h>
# include <sys/uio.h>
# include <unistd.h>
# include <fcntl.h>
#else
# error unsupported platform
#endif

namespace noldor {

#if NOLDOR_POSIX
# define EINTR_SAFE(RES, OP, ...) do RES = OP(__VA_ARGS__); while (RES == -1 && errno == EINTR);
#endif

#define REGISTERS(X) \
    X(exp) \
    X(env) \
    X(val) \
    X(proc) \
    X(argl) \
    X(continu) \
    X(unev) \
    X(compapp)

constexpr int N_REGISTERS = 8;

#define X(n) n,
enum class reg { REGISTERS(X) };
#undef X

NOLDOR_EXPORT extern const char * const regnames[N_REGISTERS];

template <class C, size_t N>
inline constexpr int array_size(C (&)[N])
{ return N; }

NOLDOR_EXPORT void list_insert(list_t *list, list_t *elem);
NOLDOR_EXPORT void list_remove(list_t *elem);

template <class C, size_t Offset>
struct looper {
    list_t * const root;

    looper(list_t *list) : root(list) {}

    static inline C* container_of_node(list_t *node)
    { return reinterpret_cast<C*>(reinterpret_cast<char *>(node) - Offset); }

    struct iterator {
        list_t * elem;
        list_t copy;

        typedef C& reference;
        typedef C* pointer;

        iterator() = delete;

        iterator(iterator &&) = default;
        iterator(const iterator &) = default;
        iterator &operator=(const iterator &) = default;
        iterator &operator=(iterator &&) = default;

        inline iterator(list_t *e) : elem(e), copy(*e) {}

        inline reference operator * () noexcept
        { return *container_of_node(elem); }

        inline pointer operator -> () noexcept
        { return container_of_node(elem); }

        inline bool operator == (iterator other) const noexcept
        { return elem == other.elem; }

        inline bool operator != (iterator other) const noexcept
        { return elem != other.elem; }

        inline iterator & operator ++ () noexcept // prefix
        { *this = iterator(copy.next); return *this; }

        inline iterator & operator -- () noexcept // prefix
        { *this = iterator(copy.prev); return *this; }

        inline iterator operator ++ (int) noexcept // postfix
        { iterator result = *this; ++(*this); return result; }

        inline iterator operator -- (int) noexcept // postfix
        { iterator result = *this; --(*this); return result; }
    };

    iterator begin()
    { return iterator { root->next }; }

    iterator end()
    { return iterator { root }; }
};

#define INTRUSIVE_LIST_LOOP(LIST, TYPE, MEMBERNAME) looper<TYPE, offsetof(TYPE, MEMBERNAME)>(LIST)

struct NOLDOR_EXPORT thread_t {
    std::vector<uint64_t> stack;
    std::array<uint64_t, N_REGISTERS> registers;

    inline value getreg(reg r)
    {
        return registers[size_t(r)];
    }

    inline void save(reg r)
    {
        stack.push_back(getreg(r));
    }

    inline void restore(reg r)
    {
        registers[size_t(r)] = stack.back();
        stack.pop_back();
    }

    inline void assign(reg r, value val)
    {
        registers[size_t(r)] = val;
    }
};

struct NOLDOR_EXPORT thread_scope_t : scope
{
    thread_t *thread = 0;

    thread_scope_t(thread_t &th) : thread(&th) {}

    void visit(gc_visit_fn_t visitor, void *data) override
    {
        if (!thread)
            return;

        for (uint64_t &regval : thread->registers) {
            value *v = reinterpret_cast<value *>(&regval);
            visitor(v, data);
        }

        for (uint64_t &stackval : thread->stack) {
            value *v = reinterpret_cast<value *>(&stackval);
            visitor(v, data);
        }
    }
};

struct NOLDOR_EXPORT gc_header {
    metatype_t *metaobject = nullptr;

    list_t gc_objects;
    uint32_t gc_flags = 0;

    uint8_t data_offset = 0;
    uint32_t alloc_size = 0;

    char data[1];
};

struct gc_status_info {
    size_t n_bytes_allocated = 0;
    size_t n_objects_allocated = 0;
};

struct NOLDOR_EXPORT globals {
    static list_t *scopes();
    static list_t *allocations();
    static struct gc_status_info *gc_status_info();
    static void register_allocation(gc_header *obj);
};

union NOLDOR_EXPORT flipper_t {
    uint64_t u64;
    double dd;
};

static_assert(sizeof(double) == sizeof(uint64_t), "unsupported platform");

class NOLDOR_EXPORT magic {
    enum : uint64_t {
        max_double = 0xfff8000000000000,
        int32_tag  = 0xfff9000000000000,
        ptr_tag    = 0xfffa000000000000,
        tag_mask   = 0xffff000000000000
    };

public:
    static inline bool is_double(uint64_t u) noexcept
    { return u <= max_double; }

    static inline bool is_int32(uint64_t u) noexcept
    { return (u & tag_mask) == int32_tag; }

    static inline bool is_pointer(uint64_t u) noexcept
    { return (u & tag_mask) == ptr_tag; }

    static inline double get_double(uint64_t u)
    { check_type(is_double, u, "magic; double expected"); flipper_t flipper; flipper.u64 = u; return flipper.dd; }

    static inline int32_t get_int32(uint64_t u)
    { check_type(is_int, u, "magic: int expected"); return static_cast<int32_t>(u & ~int32_tag); }

    static inline void* get_pointer(uint64_t u)
    { check_type(is_pointer, u, "magic: pointer expected"); return reinterpret_cast<void *>(u & ~ptr_tag); }

    static inline uint64_t from_double(double d) noexcept
    { flipper_t flipper; flipper.dd = d; return flipper.u64; }

    static inline uint64_t from_int32(int32_t i) noexcept
    { return (uint64_t(i) & 0xffffffff) | int32_tag; }

    static inline uint64_t from_pointer(const void *d) noexcept
    { return reinterpret_cast<uint64_t>(d) | ptr_tag; }
};

} // namespace noldor

#endif // NOLDOR_ECEVAL_H
