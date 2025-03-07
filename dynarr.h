#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <Windows.h>


#ifdef DYNARR_EXTERN_FUNCS
#   define DA_EXPORT   __declspec( dllexport )
#else
#   define DA_EXPORT
#endif

#ifndef DYNARR_MIN_SIZE
#   define DYNARR_MIN_SIZE 512
#endif

#ifndef DYNARR_GROW_FACTOR
#   define DYNARR_GROW_FACTOR 2
#endif

typedef struct {
    size_t count;
    size_t _used;
    size_t _capacity;
    size_t _element_size;
    void *_data;
} DynArr;


DynArr DA_EXPORT dynarr_init(size_t element_size);
void   DA_EXPORT *dynarr_alloc(DynArr *da, size_t size);
void   DA_EXPORT dynarr_shift_append(DynArr *da, void *var);
void   DA_EXPORT *dynarr_append(DynArr *da, void *var);
void   DA_EXPORT *dynarr_append_str(DynArr *da, char *str, int len);
void   DA_EXPORT *dynarr_at(DynArr *da, size_t idx);
void   DA_EXPORT dynarr_shift_append(DynArr *da, void *var);
void   DA_EXPORT *dynarr_pop(DynArr *da);
void   DA_EXPORT dynarr_reset(DynArr *da);
void   DA_EXPORT dynarr_free(DynArr *da);

#ifdef DYNARR_IMPLEMENTATION

void _dynarr_grow(DynArr *da, size_t size)
{
    HANDLE heap = GetProcessHeap(); assert(heap);
    bool needs_to_grow = false;

    while (da->_capacity < da->_used + size) {
        needs_to_grow = true;
        da->_capacity *= DYNARR_GROW_FACTOR;
    }

    if (needs_to_grow) {
        void *nptr = HeapReAlloc(heap, 0, da->_data, da->_capacity); assert(nptr != NULL);
        da->_data = nptr;
    }
}


DynArr dynarr_init(size_t element_size)
{
    HANDLE heap = GetProcessHeap(); assert(heap);

    DynArr da = {
        ._capacity = DYNARR_MIN_SIZE,
        ._element_size = element_size,
        ._data = HeapAlloc(heap, 0, DYNARR_MIN_SIZE),
    };

    assert(da._data != NULL);

    return da;
}


/*
 * To provide buf for string ops
*/
void *dynarr_alloc(DynArr *da, size_t size)
{
    void *ptr = (char *)da->_data + da->_used;
    _dynarr_grow(da, size);
    da->count += size;
    da->_used += size;

    return ptr;
}


void *dynarr_append(DynArr *da, void *var)
{
    _dynarr_grow(da, da->_element_size);

    void *ptr_to_save = (char *)da->_data + da->_used;
    memcpy(ptr_to_save, var, da->_element_size);

    da->count++;
    da->_used += da->_element_size;

    return ptr_to_save;
}


void *dynarr_append_str(DynArr *da, char *str, int len)
{
    if (len < 0) len = (int)strlen(str);

    _dynarr_grow(da, len);

    void *ptr_to_save = (char *)da->_data + da->_used;
    memcpy(ptr_to_save, str, len);
    da->count += len;
    da->_used += len;

    return ptr_to_save;
}


void *dynarr_at(DynArr *da, size_t idx)
{
    if (idx < 0 || idx >= da->count)
        return NULL;

    return (char *)da->_data + (da->_element_size * idx);
}


void dynarr_shift_append(DynArr *da, void *var)
{
    if (!da->count) return;

    for (int i = 1; i < da->count; ++i) {
        memcpy(dynarr_at(da, i - 1), dynarr_at(da, i), da->_element_size);
    }
    memcpy(dynarr_at(da, da->count - 1), var, da->_element_size);
}


void *dynarr_pop(DynArr *da)
{
    da->count--;
    da->_used -= da->_element_size;
    return (char *)da->_data + (da->_element_size * da->count);
}


void dynarr_reset(DynArr *da)
{
    da->count = 0;
    da->_used = 0;
}


void dynarr_free(DynArr *da)
{
    HANDLE heap = GetProcessHeap(); assert(heap);
    HeapFree(heap, 0, da->_data);
    da->count = 0;
    da->_used = 0;
    da->_element_size = 0;
    da->_capacity = 0;
    da->_data = 0;
}
#endif  // DYNARR_IMPLEMENTATION
