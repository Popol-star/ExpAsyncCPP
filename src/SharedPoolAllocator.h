#pragma once
#include <memory_resource>
inline std::pmr::synchronized_pool_resource POOLED_ALLOCATOR;
template <class T>
class PooledAlloc {
public:
    // type definitions
    typedef T  value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;

    // rebind allocator to type U
    template <class U>
    struct rebind {
        typedef PooledAlloc<U> other;
    };

    // return address of values
    pointer address(reference value) const {
        return &value;
    }
    const_pointer address(const_reference value) const {
        return &value;
    }
    PooledAlloc(){
    }
    PooledAlloc(const PooledAlloc&) noexcept {
    }
    template <class U>
    PooledAlloc(const PooledAlloc<U>&) noexcept {
    }
    ~PooledAlloc()  {
    }

    size_type max_size() const noexcept {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }

    pointer allocate(size_type num, const void* = 0) {
        // print message and allocate memory with global new
        return (pointer)POOLED_ALLOCATOR.allocate(num * sizeof(value_type), alignof(value_type));
    }
    void construct(pointer p, const T& value) {
        // initialize memory with placement new
        new((void*)p)T(value);
    }
    void destroy(pointer p) {
        p->~T();
    }

    void deallocate(pointer p, size_type num) {
        POOLED_ALLOCATOR.deallocate(p, num * sizeof(value_type), alignof(value_type));
    }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
inline bool operator== (const PooledAlloc<T1>&,
    const PooledAlloc<T2>&) noexcept {
    return true;
}
template <class T1, class T2>
inline bool operator!= (const PooledAlloc<T1>&,
    const PooledAlloc<T2>&) noexcept {
    return false;
}

