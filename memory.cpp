#include "noldor_impl.h"
#include <cassert>

namespace noldor {

static void gc_mark_recursive(value *val, void *data)
{
    if (!magic::is_pointer(*val))
        return;

    auto header = static_cast<gc_header *>(magic::get_pointer(*val));
    const uint32_t mark = *reinterpret_cast<uint32_t *>(data);

    if (header->gc_flags == mark)
        return;

    header->gc_flags = mark;

    if (header->metaobject->gc_visit)
        header->metaobject->gc_visit(*val, gc_mark_recursive, data);
}

int run_gc()
{
    list_t *allocations = globals::allocations();
    list_t *scopes = globals::scopes();

    uint32_t dead_mark = 0;
    uint32_t live_mark = 1;

    for (gc_header &header : INTRUSIVE_LIST_LOOP(allocations, gc_header, gc_objects))
        header.gc_flags = dead_mark;

    for (scope_data &scope : INTRUSIVE_LIST_LOOP(scopes, scope_data, gc_scopes))
        for (value *var : scope.variables)
            gc_mark_recursive(var, &live_mark);

    int n_bytes_freed = 0;

    for (gc_header &header : INTRUSIVE_LIST_LOOP(allocations, gc_header, gc_objects)) {
        if (header.gc_flags == dead_mark) {
            assert(header.metaobject);

            n_bytes_freed += header.alloc_size + header.data_offset;

            if (header.metaobject->destruct)
                header.metaobject->destruct(magic::from_pointer(&header));

            list_remove(&header.gc_objects);
            free(&header);
        }
    }

    return n_bytes_freed;
}

value allocate(metatype_t *metaobject, size_t size, size_t alignment)
{
    size_t header_size = sizeof(gc_header) - 1;
    size_t padding = 0;

    if (size > 0 && alignment > 0)
        padding = alignment - 1;

    auto *mem = static_cast<char *>(malloc(header_size + padding + size));
    auto header = new (mem) gc_header;

    void *object = header->data;
    size_t object_space = padding + size;
    std::align(alignment, size, object, object_space);

    auto offset = header_size + padding + size - object_space;

    assert(offset <= UINT8_MAX);
    assert(size < UINT32_MAX);

    header->metaobject = metaobject;
    header->data_offset = uint8_t(offset);
    header->alloc_size = uint32_t(size);

    globals::register_allocation(header);
    return magic::from_pointer(mem);
}

scope::scope() : data(new scope_data)
{
    list_insert(globals::scopes(), &data->gc_scopes);
}

scope::~scope()
{
    list_remove(&data->gc_scopes);
}

void scope::add(value &v)
{
    for (const value *vv : data->variables)
        if (eq(v, *vv))
            return;

    data->variables.emplace_back(&v);
}

void scope::remove(value &v)
{
    auto i = data->variables.begin();

    while (i != data->variables.end()) {
        if (eq(v, **i)) {
            data->variables.erase(i);
        } else {
            ++i;
        }
    }
}

list_t *globals::scopes()
{
    static list_t scopes;
    return &scopes;
}

list_t *globals::allocations()
{
    static list_t allocs;
    return &allocs;
}

void globals::register_allocation(gc_header *obj)
{
    list_insert(globals::allocations(), &obj->gc_objects);
}

void *object_data(value obj)
{
    if (!magic::is_pointer(obj))
        return nullptr;

    auto header = static_cast<gc_header *>(magic::get_pointer(obj));
    return reinterpret_cast<char *>(header) + header->data_offset;
}

metatype_t *object_metaobject(value obj)
{
    if (!magic::is_pointer(obj))
        return nullptr;

    return static_cast<metatype_t *>(magic::get_pointer(obj))->self;
}

} // namespace noldor
