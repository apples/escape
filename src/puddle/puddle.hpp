#ifndef PUDDLE_PUDDLE_HPP
#define PUDDLE_PUDDLE_HPP

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace Puddle {

namespace _detail {

static constexpr auto ERR_ONLY_ONE =
    "Puddle: Cannot (de)allocate more than one object at once!";

template <typename T, bool E>
class Allocator;

template <typename T>
using Allocator_t = Allocator<T, ::std::is_empty<T>::value>;

template <typename T>
class Allocator<T, false>
{
    public:

        template <typename U>
        struct rebind
        {
            using other = Allocator_t<U>;
        };

        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = ::std::size_t;

    private:

        struct Global
        {
            struct Element
            {
                union
                {
                    value_type val;
                    Element* next;
                };

                Element()
                    : next()
                {}

                ~Element()
                {}
            };

            static constexpr size_type ELE_SIZE = sizeof(Element);
            static constexpr size_type BLK_SIZE = 8U * 1024U;
            static constexpr size_type BLK_ELES = BLK_SIZE/ELE_SIZE;

            struct Block
            {
                Element eles[BLK_ELES];
            };

            ::std::vector<::std::unique_ptr<Block>> blocks;
            Element* next = nullptr;

            T* allocate()
            {
                if (!next)
                {
                    ::std::unique_ptr<Block> ptr (new Block);

                    next = &ptr->eles[0];

                    for (size_type i=0; i<BLK_ELES-1; ++i)
                    {
                        ptr->eles[i].next = &ptr->eles[i+1];
                    }

                    ptr->eles[BLK_ELES-1].next = nullptr;

                    blocks.emplace_back(::std::move(ptr));
                }

                T* rv = &next->val;

                next = next->next;

                return rv;
            }

            void deallocate(T* t)
            {
                Element* curr = reinterpret_cast<Element*>(t);
                curr->next = next;
                next = curr;
            }
        };

        static Global& getGlobal()
        {
            static Global inst;
            return inst;
        }

    public:

        Allocator() = default;

        template <typename U, bool C>
        Allocator(Allocator<U,C> const&)
            : Allocator()
        {}

        size_type max_size() const
        {
            return 1;
        }

        T* allocate(size_type one)
        {
            if (one != 1)
                throw ::std::runtime_error(ERR_ONLY_ONE);
            return getGlobal().allocate();
        }

        void deallocate(T* t, size_type one)
        {
            if (one != 1)
                throw ::std::runtime_error(ERR_ONLY_ONE);
            return getGlobal().deallocate(t);
        }

        template <typename U, typename... Args>
        void construct(U* p, Args&&... args)
        {
            ::new (p) U(::std::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U* p)
        {
            p->~U();
        }
};

template <typename T>
class Allocator<T, true>
{
    public:
        template <typename U>
        struct rebind
        {
            using other = Allocator_t<U>;
        };

        using value_type = T;
        using size_type = ::std::size_t;

        T* allocate(size_type, void*)
        {
            return nullptr;
        }

        void deallocate(T*, size_type)
        {}
};

} // namespace _detail

template <typename T>
using Allocator = _detail::Allocator_t<T>;

} // namespace Puddle

#endif // PUDDLE_PUDDLE_HPP
