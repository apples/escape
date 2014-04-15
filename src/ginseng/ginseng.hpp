#ifndef GINSENG_HPP
#define GINSENG_HPP

#include <cstdint>
#include <algorithm>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>

namespace Ginseng {

// Base IDs

using EID = ::std::int_fast64_t; // Entity ID
using CID = ::std::int_fast64_t; // Component ID
using TID = ::std::int_fast64_t; // Table ID

namespace _detail {

// Type Info

    inline TID nextTID()
    {
        static TID tid = 0;
        return ++tid;
    }

// Component Tags

    template <typename T>
    struct Not
    {
        static TID getTID()
        {
            return -T::getTID();
        }
    };

// Traits

    template <typename... Ts>
    struct TypeList
    {};

    // TypeListCat

        template <typename T, typename U>
        struct TypeListCat;

        template <typename... Ts, typename... Us>
        struct TypeListCat<TypeList<Ts...>, TypeList<Us...>>
        {
            using type = TypeList<Ts..., Us...>;
        };

        template <typename... Ts>
        using TypeListCat_t = typename TypeListCat<Ts...>::type;

    template <typename... Ts>
    struct Count
        : ::std::integral_constant<int, ::std::tuple_size<::std::tuple<Ts*...>>::value>
    {};

    // IsNot

        template <typename>
        struct IsNot;

        template <typename T>
        struct IsNot
            : ::std::false_type
        {};

        template <typename T>
        struct IsNot<Not<T>>
            : ::std::true_type
        {};

    // RemoveNot

        template <typename...>
        struct RemoveNot;

        template <typename T, typename... Us>
        struct RemoveNot<T, Us...>
        {
            using type = TypeListCat_t<TypeList<T>, typename RemoveNot<Us...>::type>;
        };

        template <typename T, typename... Us>
        struct RemoveNot<Not<T>, Us...>
        {
            using type = typename RemoveNot<Us...>::type;
        };

        template <>
        struct RemoveNot<>
        {
            using type = TypeList<>;
        };

        template <typename... Ts>
        using RemoveNot_t = typename RemoveNot<Ts...>::type;

    // GetNots

        template <typename...>
        struct GetNots;

        template <typename T, typename... Us>
        struct GetNots<T, Us...>
        {
            using type = typename GetNots<Us...>::type;
        };

        template <typename T, typename... Us>
        struct GetNots<Not<T>, Us...>
        {
            using type = typename TypeListCat<TypeList<T>, typename GetNots<Us...>::type>::type;
        };

        template <>
        struct GetNots<>
        {
            using type = TypeList<>;
        };

    // AddPointer

        template <typename...>
        struct AddPointer;

        template <template <typename...> class Tup, typename... Ts>
        struct AddPointer<Tup<Ts...>>
        {
            using type = Tup<Ts*...>;
        };

        template <typename... Ts>
        using AddPointer_t = typename AddPointer<Ts...>::type;

    // ToTuple

        template <typename...>
        struct ToTuple;

        template <template <typename...> class Tup, typename... Ts>
        struct ToTuple<Tup<Ts...>>
        {
            using type = ::std::tuple<Ts...>;
        };

        template <typename... Ts>
        using ToTuple_t = typename ToTuple<Ts...>::type;

    // Filters

        // IsOneOf

            template <typename, typename...>
            struct IsOneOf;

            template <typename T, typename U, typename... Vs>
            struct IsOneOf<T, U, Vs...>
                : IsOneOf<T, Vs...>::value
            {};

            template <typename T, typename... Vs>
            struct IsOneOf<T, T, Vs...>
                : ::std::true_type
            {};

            template <typename T>
            struct IsOneOf<T>
                : ::std::false_type
            {};

// Containers

    template <typename T, typename H = ::std::hash<T>>
    using Many = ::std::unordered_set<T, H>;

    template <typename K, typename V, typename H = ::std::hash<K>>
    using Table = ::std::unordered_map<K, V, H>;

    template <typename K, typename V>
    using SlowTable = ::std::map<K, V>;

    template <typename T>
    using Vec = ::std::vector<T>;

    // Quick Erasure

        struct ErasureBase
        {
            virtual ~ErasureBase() = 0;
        };

        inline ErasureBase::~ErasureBase()
        {}

        template <typename T>
        struct Erase
            : ErasureBase
        {
            T t;

            template <typename... Us>
            Erase(Us&&... us)
                : t(::std::forward<Us>(us)...)
            {}
        };

// Component Base

    class ComponentBase
    {
    protected:
        ComponentBase() = default;
        ComponentBase(ComponentBase const&) = default;
        ComponentBase(ComponentBase&&) noexcept = default;
    public:
        virtual ~ComponentBase() = 0;
        virtual ::std::unique_ptr<ComponentBase> clone() const = 0;
    };

    inline ComponentBase::~ComponentBase()
    {}

// Component CRTP

    template <typename Child>
    class ComponentDat
        : public ComponentBase
    {
        friend class Database;
    public:
        static TID getTID()
        {
            static TID tid = nextTID();
            return tid;
        }
    };

    template <typename Child, bool Factory = true>
    class Component;

    template <typename Child>
    class Component<Child, true>
        : public ComponentDat<Child>
    {
    public:
        virtual ::std::unique_ptr<ComponentBase> clone() const override
        {
            return ::std::unique_ptr<ComponentBase>(new Child(reinterpret_cast<const Child&>(*this)));
        }
    };

    template <typename Child>
    class Component<Child, false>
        : public ComponentDat<Child>
    {};

// Types

    class Database;

    namespace Types
    {
        template <typename T>
        struct EntityData
        {
            Table<TID, typename Table<CID,T>::iterator> coms;
        };

        template <typename T>
        class Entity
        {
            friend class _detail::Database;
            using Iter = typename Table<EID,EntityData<T>>::iterator;

            Iter iter;

            Entity(Iter i) : iter(i) {}

        public:
            struct Hash
            {
                ::std::hash<EID>::result_type operator()(Entity const& e) const
                {
                    return ::std::hash<EID>{}(e.getID());
                }
            };

            Entity() = default;

            bool operator<(Entity const& e) const
            {
                return (iter->first < e.iter->first);
            }

            bool operator==(Entity const& e) const
            {
                return (iter->first == e.iter->first);
            }

            EID getID() const
            {
                return iter->first;
            }
        };

        struct ComponentData
        {
            ::std::unique_ptr<Entity<ComponentData>> ent; // Disgusting, this shouldn't have to be a pointer.
            ::std::unique_ptr<ComponentBase> com;
            ComponentData(Entity<ComponentData> const& e, ::std::unique_ptr<ComponentBase>&& c)
                : ent(new Entity<ComponentData>(e))
                , com(::std::move(c))
            {}
        };

        struct ComponentTable
        {
            Table<CID,ComponentData> list;
            Many<Entity<ComponentData>, Entity<ComponentData>::Hash> ents;
        };
    }

    using Entity         = Types::Entity<Types::ComponentData>;
    using EntityData     = Types::EntityData<Types::ComponentData>;
    using ComponentData  = Types::ComponentData;
    using ComponentTable = Types::ComponentTable;

// Query results

    template <typename... Ts>
    using RElement =
        ToTuple_t
        <
            TypeListCat_t
            <
                TypeList<Entity>,
                AddPointer_t< RemoveNot_t<Ts...> >
            >
        >;

    template <typename... Ts>
    using Result = ::std::vector<RElement<Ts...>>;

// Main Database engine

    class Database
    {
        Table<EID,EntityData> entities;
        Table<TID,ComponentTable> components;
        SlowTable<Vec<TID>,::std::unique_ptr<ErasureBase>> memos;

        struct
        {
            EID eid = 0;
            CID cid = 0;
        } uidGen;

        EID createEntityID()
        {
            return ++uidGen.eid;
        }

        CID createCID()
        {
            return ++uidGen.cid;
        }

    // Helpers

        // fill_inspect
            // Fills the tuple with components from N by inspecting the entity.

            template <typename U, int N = 2, typename T, int M = N-::std::tuple_size<T>::value>
            typename ::std::enable_if<
                (N < ::std::tuple_size<T>::value),
            bool>::type fill_inspect(T& ele) const
            {
                using PtrType = typename ::std::tuple_element<N, T>::type;
                using Type = typename ::std::remove_pointer<PtrType>::type;

                auto const& coms = ::std::get<0>(ele).iter->second.coms;
                auto iter = coms.find(Type::getTID());

                if (iter == coms.end())
                    return false;

                ::std::get<N>(ele) = static_cast<PtrType>(iter->second->second.com.get());

                return fill_inspect<U, N+1>(ele);
            }

            template <typename U, int N = 2, typename T, int M = N-::std::tuple_size<T>::value>
            typename ::std::enable_if<
                (N >= ::std::tuple_size<T>::value) and
                (M <  ::std::tuple_size<U>::value),
            bool>::type fill_inspect(T& ele) const
            {
                using PtrType = typename ::std::tuple_element<M, U>::type;
                using Type = typename ::std::remove_pointer<PtrType>::type;

                auto const& coms = ::std::get<0>(ele).iter->second.coms;
                auto iter = coms.find(Type::getTID());

                if (iter != coms.end())
                    return false;

                return fill_inspect<U, N+1>(ele);
            }

            template <typename U, int N = 2, typename T, int M = N-::std::tuple_size<T>::value>
            typename ::std::enable_if<
                (N >= ::std::tuple_size<T>::value) and
                (M >= ::std::tuple_size<U>::value),
            bool>::type fill_inspect(T& ele) const
            {
                return true;
            }

        // select_inspect
            // Gathers all entities containing the first component, forwards to fill_inspect.

            template <typename T, typename... Us>
            Result<T, Us...> select_inspect() const
            {
            static_assert(not IsNot<T>::value, "First component must be positive!");

                Result<T, Us...> rval;

                auto iter = components.find(T::getTID());
                if (iter == components.end())
                    return rval;

                const ComponentTable& table = iter->second;

                for (auto const& p : table.list)
                {
                    const ComponentData& cd = p.second;
                    T* data = static_cast<T*>(cd.com.get());

                    RElement<T, Us...> ele;

                    ::std::get<0>(ele) = *cd.ent;
                    ::std::get<1>(ele) = data;

                    if (fill_inspect<typename ToTuple<typename GetNots<Us...>::type>::type>(ele))
                    {
                        rval.emplace_back(::std::move(ele));
                    }
                }

                return rval;
            }

        // fill_memo_vec

            template <int, typename...>
            struct fill_memo_vec;

            template <int I, typename T, typename... Us>
            struct fill_memo_vec<I, T, Us...>
            {
                static void func(Vec<TID>& vt)
                {
                    vt.push_back(T::getTID());
                    return fill_memo_vec<I+1, Us...>::func(vt);
                }
            };

            template <int I>
            struct fill_memo_vec<I>
            {
                static void func(Vec<TID>& vt)
                {}
            };

        // fill_components

            template <int I = 0, typename... Ts>
            static typename ::std::enable_if<
                I < Count<Ts...>::value,
            void>::type fill_components(::std::tuple<Ts*...>& tup, decltype(EntityData::coms) const& etab)
            {
                using C = typename ::std::tuple_element<I, ::std::tuple<Ts...>>::type;
                TID tid = C::getTID();

                auto iter = etab.find(tid);
                if (iter != etab.end())
                {
                    ::std::get<I>(tup) = static_cast<C*>(iter->second->second.com.get());
                }

                return fill_components<I+1>(tup, etab);
            }

            template <int I = 0, typename... Ts>
            static typename ::std::enable_if<
                I >= Count<Ts...>::value,
            void>::type fill_components(::std::tuple<Ts*...>& tup, decltype(EntityData::coms) const& etab)
            {}

    public:

        enum class Selector
        {
            INSPECT
        };

        template <typename... Ts>
        static ::std::tuple<Ts*...> getComponents(Entity ent)
        {
            ::std::tuple<Ts*...> rv;

            fill_components<0>(rv, ent.iter->second.coms);

            return rv;
        }

        Entity newEntity()
        {
            EID eid = createEntityID();
            return entities.emplace(eid, EntityData{}).first;
        }

        Entity cloneEntity(Entity ent)
        {
            Entity rv = newEntity();

            Vec<TID> tids;

            for (const auto& p : ent.iter->second.coms)
            {
                auto tid = p.first;

                tids.push_back(tid);

                ComponentBase const* com = p.second->second.com.get();

                auto& table = components.at(tid);

                auto newcid = createCID();
                auto newcom = com->clone();

                ComponentData cd (rv, ::std::move(newcom));

                auto comiter = table.list.emplace(newcid, ::std::move(cd)).first;

                rv.iter->second.coms.emplace(tid, comiter);

                table.ents.insert(rv);
            }

            for (auto i=memos.begin(), ie=memos.end(); i!=ie;)
            {
                bool hit = false;
                for (auto&& a : i->first)
                    for (auto&& b : tids)
                        if (a==b)
                        {
                            hit = true;
                            break;
                        }
                if (hit) i = memos.erase(i);
                else ++i;
            }

            return rv;
        }

        void eraseEntity(Entity ent)
        {
            memos.clear();

            for (auto&& p : ent.iter->second.coms)
            {
                ComponentTable& tab = components.at(p.first);
                tab.list.erase(tab.list.find(p.second->first));
                tab.ents.erase(tab.ents.find(ent));
            }

            entities.erase(ent.iter);
        }

        template <typename T, typename... Vs>
        T& newComponent(Entity ent, Vs&&... vs)
        {
            TID tid = T::getTID();

            for (auto i=memos.begin(), ie=memos.end(); i!=ie;)
            {
                auto a = i->first.begin();
                auto b = i->first.end();
                auto iter = ::std::find(a, b, tid);
                if (iter != b) i = memos.erase(i);
                else ++i;
            }

            CID cid = createCID();

            ::std::unique_ptr<_detail::ComponentBase> ptr (new T(::std::forward<Vs>(vs)...));
            T& rval = *static_cast<T*>(ptr.get());

            ComponentTable& table = components[tid];
            ComponentData dat { ent, ::std::move(ptr) };

            auto comiter = table.list.emplace(cid, ::std::move(dat)).first;
            table.ents.insert(ent);

            ent.iter->second.coms.emplace(tid, comiter);

            return rval;
        }

        template <typename... Ts>
        Result<Ts...> const& getEntities(const Selector method = Selector::INSPECT)
        {
            auto unerase = [](decltype(memos.begin()) iter)-> Result<Ts...>&
            {
                return static_cast<Erase<Result<Ts...>>*>(iter->second.get())->t;
            };

            Vec<TID> vt;

            vt.reserve(Count<Ts...>::value);
            fill_memo_vec<0, Ts...>::func(vt);
            ::std::stable_partition(::std::begin(vt), ::std::begin(vt), [](TID tid){return (tid>0);});

            auto iter = memos.find(vt);
            if (iter != memos.end())
                return unerase(iter);

            Result<Ts...> rv;
            ::std::unique_ptr<ErasureBase> up;

            switch (method)
            {
                case Selector::INSPECT:
                    rv = select_inspect<Ts...>();
                    up.reset(new Erase<Result<Ts...>>(::std::move(rv)));
                    iter = memos.emplace(::std::move(vt), ::std::move(up)).first;
                    break;

                default:
                    throw; //TODO
            }

            return unerase(iter);
        }

        decltype(entities.size()) numEntities() const
        {
            return entities.size();
        }
    };

} // namespace _detail

// Public types

    template <typename Child, bool Factory = true>
    using Component = _detail::Component<Child, Factory>;

    using Entity   = _detail::Entity;
    using Database = _detail::Database;

    template <typename T>
    using Not = _detail::Not<T>;

// Public interface

    template <typename... Ts>
    ::std::tuple<Ts*...> getComponents(Entity ent)
    {
        return Database::getComponents<Ts...>(ent);
    }

} // namespace Ginseng

#endif
