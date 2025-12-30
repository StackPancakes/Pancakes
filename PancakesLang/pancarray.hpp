#ifndef PANCARRAY_HPP
#define PANCARRAY_HPP

#include <cstddef>
#include <cassert>

namespace panc
{
    template<typename T, std::size_t Capacity>
    struct array
    {
        T data[Capacity]{};
        std::size_t count{ 0 };

        void push_back(T const& v)
        {
            if (count < Capacity)
                data[count++] = v;
        }

        void pop_back()
        {
            if (count > 0)
                --count;
        }

        void clear()
        {
            count = 0;
        }

        [[nodiscard]] bool empty() const
        {
            return count == 0;
        }

        [[nodiscard]] std::size_t size() const
        {
            return count;
        }

        T& back()
        {
            assert(count > 0);
            return data[count - 1];
        }

        [[nodiscard]] T const& back() const
        {
            return data[count - 1];
        }

        T& operator[](std::size_t i)
        {
            return data[i];
        }

        T const& operator[](std::size_t i) const
        {
            return data[i];
        }

        T const* begin()
        {
            return data;
        }

        T const* end()
        {
            return data + count;
        }
    };
}

#endif