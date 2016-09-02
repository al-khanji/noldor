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

    for (scope &sc : INTRUSIVE_LIST_LOOP(scopes, scope, gc_scopes))
        sc.visit(gc_mark_recursive, &live_mark);

    auto *gc_status = globals::gc_status_info();

    int n_bytes_freed = 0;

    for (gc_header &header : INTRUSIVE_LIST_LOOP(allocations, gc_header, gc_objects)) {
        if (header.metaobject->flags & typeflags_static)
            continue;

        if (header.gc_flags == dead_mark) {
            assert(header.metaobject);

            n_bytes_freed += header.alloc_size + header.data_offset;

            list_remove(&header.gc_objects);

            if (header.metaobject->destruct)
                header.metaobject->destruct(magic::from_pointer(&header));

            free(&header);

            gc_status->n_objects_allocated -= 1;
        }
    }

    gc_status->n_bytes_allocated -= size_t(n_bytes_freed);
    return n_bytes_freed;
}

value allocate(metatype_t *metaobject, size_t size, size_t alignment)
{
    auto *gc_status = globals::gc_status_info();

    size_t header_size = sizeof(gc_header) - 1;
    size_t padding = 0;

    if (size > 0 && alignment > 0)
        padding = alignment - 1;

    size_t total_size = header_size + padding + size;

    auto *mem = static_cast<char *>(malloc(total_size));
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

    gc_status->n_bytes_allocated += total_size;
    gc_status->n_objects_allocated += 1;

    return magic::from_pointer(mem);
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

struct gc_status_info *globals::gc_status_info()
{
    static struct gc_status_info info;
    return &info;
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

    auto header = static_cast<gc_header *>(magic::get_pointer(obj));
    return header->metaobject;
}

scope::scope()
{
    list_insert(globals::scopes(), &gc_scopes);
}


scope::~scope()
{
    list_remove(&gc_scopes);
}


} // namespace noldor
