#ifndef NOLDOR_ECEVAL_H
#define NOLDOR_ECEVAL_H

#include "noldor.h"

#include <iostream>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>

namespace noldor {

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

struct NOLDOR_EXPORT list_t {
    list_t *next = this;
    list_t *prev = this;
};

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

struct NOLDOR_EXPORT scope_data {
    list_t gc_scopes;
    std::vector<value *> variables;
};

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

struct NOLDOR_EXPORT symbol_t {
    std::string name;
    std::size_t hash;
};

struct NOLDOR_EXPORT pair_t {
    uint64_t car;
    uint64_t cdr;
};

struct NOLDOR_EXPORT environment_t {
    value outer = list();
    std::unordered_map<uint64_t, uint64_t> symtab;
};

struct NOLDOR_EXPORT primitive_function_t {
    std::string name;
    std::function<value(value)> fn;
};

struct NOLDOR_EXPORT compound_procedure_t {
    uint64_t environment;
    uint64_t parameters;
    uint64_t body;
};

struct NOLDOR_EXPORT vector_t {
    std::vector<value> elements;
};

struct NOLDOR_EXPORT string_t {
    std::string string;
};

struct NOLDOR_EXPORT char_t {
    uint32_t character;
};

struct NOLDOR_EXPORT gc_header {
    metatype_t *metaobject = nullptr;

    list_t gc_objects;
    uint32_t gc_flags = 0;

    uint8_t data_offset = 0;
    uint32_t alloc_size = 0;

    char data[1];
};

struct NOLDOR_EXPORT gc_mark_sweep_data {
    uint32_t mark;
    std::unordered_set<gc_header *> pending_garbage;
};

struct NOLDOR_EXPORT globals {
    static list_t *scopes();
    static list_t *allocations();

    static std::unordered_set<metatype_t *> &metaobjects();
    static std::unordered_map<std::size_t, uint64_t> &symbols();

    static void register_allocation(gc_header *obj);
    static void register_type(metatype_t *metaobject);

    static value intern_symbol(const std::string &name);

    static value global_environment();

    static metatype_t *false_metaobject();
    static metatype_t *true_metaobject();
    static metatype_t *null_metaobject();
    static metatype_t *pair_metaobject();
    static metatype_t *environment_metaobject();
    static metatype_t *primitive_function_metaobject();
    static metatype_t *primitive_macro_metaobject();
    static metatype_t *compound_function_metaobject();
    static metatype_t *vector_metaobject();
    static metatype_t *string_metaobject();
    static metatype_t *char_metaobject();
    static metatype_t *eof_metaobject();
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
        symb_tag   = 0xfffb000000000000,
        tag_mask   = 0xffff000000000000
    };

public:
    static inline bool is_double(uint64_t u) noexcept
    { return u <= max_double; }

    static inline bool is_int32(uint64_t u) noexcept
    { return (u & tag_mask) == int32_tag; }

    static inline bool is_pointer(uint64_t u) noexcept
    { return (u & tag_mask) == ptr_tag; }

    static inline bool is_symbol(uint64_t u) noexcept
    { return (u & tag_mask) == symb_tag; }

    static inline double get_double(uint64_t u)
    { check_type(is_double, u, "magic; double expected"); flipper_t flipper; flipper.u64 = u; return flipper.dd; }

    static inline int32_t get_int32(uint64_t u)
    { check_type(is_int, u, "magic: int expected"); return static_cast<int32_t>(u & ~int32_tag); }

    static inline void* get_pointer(uint64_t u)
    { check_type(is_pointer, u, "magic: pointer expected"); return reinterpret_cast<void *>(u & ~ptr_tag); }

    static inline symbol_t* get_symbol(uint64_t u)
    { check_type(is_symbol, u, "magic: symbol expected"); return reinterpret_cast<symbol_t *>(u & ~symb_tag); }

    static inline uint64_t from_double(double d) noexcept
    { flipper_t flipper; flipper.dd = d; return flipper.u64; }

    static inline uint64_t from_int32(int32_t i) noexcept
    { return (uint64_t(i) & 0xffffffff) | int32_tag; }

    static inline uint64_t from_pointer(const void *d) noexcept
    { return reinterpret_cast<uint64_t>(d) | ptr_tag; }

    static inline uint64_t from_symbol(const symbol_t *sym) noexcept
    { return reinterpret_cast<uint64_t>(sym) | symb_tag; }
};

} // namespace noldor

#endif // NOLDOR_ECEVAL_H
