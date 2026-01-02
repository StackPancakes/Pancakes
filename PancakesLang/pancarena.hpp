#ifndef PANCARENA_HPP
#define PANCARENA_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <new>
#include <utility>
#include "pancarray.hpp"
#include "pancdef.hpp"

namespace panc
{
    struct Arena
    {
        alignas(std::max_align_t)
            panc::array<std::byte, MAX_CAPACITY_SIZE> memory{};

        std::size_t offset{};
        std::size_t dstack_offset{ MAX_CAPACITY_SIZE };

    private:
        struct DestructorEntry
        {
            void (*destroy)(void*, std::size_t);
            void* ptr;
            std::size_t count;
        };

        template <typename U>
        static void destroy_range(void* p, std::size_t n)
        {
            U* up{ static_cast<U*>(p) };
            for (std::size_t i{ n }; i-- > 0; )
                up[i].~U();
        }

        static constexpr std::size_t align_down(std::size_t val, std::size_t align)
        {
            return val & ~(align - 1);
        }

        bool push_destructor(DestructorEntry const& entry)
        {
            std::size_t const entrySize{ sizeof(DestructorEntry) };
            std::size_t const align{ alignof(DestructorEntry) };
            std::size_t new_doffset{ dstack_offset };
            if (new_doffset < entrySize)
                return false;
            new_doffset -= entrySize;
            new_doffset = align_down(new_doffset, align);
            if (offset > new_doffset)
                return false;
            dstack_offset = new_doffset;
            DestructorEntry* dest{ reinterpret_cast<DestructorEntry*>(memory.data + dstack_offset) };
            *dest = entry;
            return true;
        }

    public:
        Arena() : offset{}, dstack_offset{ MAX_CAPACITY_SIZE } {}

        ~Arena()
        {
            reset();
        }

        template<typename T, typename... Args>
        T* allocateTrivial(Args&&... args)
        {
            static_assert(std::is_trivially_destructible_v<T>,
                "allocateTrivial only supports trivially destructible types");

            std::size_t const align{ alignof(T) };
            std::uintptr_t const base{ reinterpret_cast<std::uintptr_t>(memory.data) + offset };
            std::size_t const adjust{ (align - (base % align)) % align };
            std::size_t const newOffset{ offset + adjust + sizeof(T) };

            if (newOffset > dstack_offset)
                return nullptr;

            std::byte* raw{ memory.data + offset + adjust };
            offset = newOffset;
            return new (raw) T(std::forward<Args>(args)...);
        }

        template<typename T, typename... Args>
        T* allocate(Args&&... args)
        {
            static_assert(std::is_nothrow_constructible_v<T, Args...>,
                "Arena allocation requires nothrow-constructible types in freestanding mode");

            std::size_t const align{ alignof(T) };
            std::uintptr_t const base{ reinterpret_cast<std::uintptr_t>(memory.data) + offset };
            std::size_t const adjust{ (align - (base % align)) % align };
            std::size_t const newOffset{ offset + adjust + sizeof(T) };

            if (newOffset > dstack_offset)
                return nullptr;

            std::byte* raw{ memory.data + offset + adjust };
            offset = newOffset;

            T* obj{ new (raw) T(std::forward<Args>(args)...) };

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                DestructorEntry entry{ &destroy_range<T>, static_cast<void*>(obj), 1 };
                if (!push_destructor(entry))
                {
                    obj->~T();
                    offset -= (adjust + sizeof(T));
                    return nullptr;
                }
            }

            return obj;
        }

        template<typename T, typename... Args>
        T* allocateArray(std::size_t count, Args&&... args)
        {
            if (count == 0)
                return nullptr;

            static_assert(std::is_nothrow_constructible_v<T, Args...>,
                "Arena allocation requires nothrow-constructible types in freestanding mode");

            std::size_t const align{ alignof(T) };
            std::uintptr_t const base{ reinterpret_cast<std::uintptr_t>(memory.data) + offset };
            std::size_t const adjust{ (align - (base % align)) % align };
            std::size_t const totalSize{ sizeof(T) * count };

            if (offset + adjust + totalSize > dstack_offset)
                return nullptr;

            std::byte* raw{ memory.data + offset + adjust };
            offset += adjust + totalSize;

            T* start{ reinterpret_cast<T*>(raw) };
            for (std::size_t i{ 0 }; i < count; ++i)
                new (raw + i * sizeof(T)) T(std::forward<Args>(args)...);

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                DestructorEntry entry{ &destroy_range<T>, static_cast<void*>(start), count };
                if (!push_destructor(entry))
                {
                    for (std::size_t i{ count }; i-- > 0; )
                        start[i].~T();
                    offset -= (adjust + totalSize);
                    return nullptr;
                }
            }

            return start;
        }

        void reset()
        {
            DestructorEntry* it{ reinterpret_cast<DestructorEntry*>(memory.data + dstack_offset) };
            DestructorEntry* const end{ reinterpret_cast<DestructorEntry*>(memory.data + MAX_CAPACITY_SIZE) };
            while (it < end)
            {
                it->destroy(it->ptr, it->count);
                ++it;
            }
            offset = 0;
            dstack_offset = MAX_CAPACITY_SIZE;
        }

        std::size_t used() const
        {
            return offset;
        }

        std::size_t capacity() const
        {
            return MAX_CAPACITY_SIZE;
        }
    };
}

#endif
