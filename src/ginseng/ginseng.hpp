#ifndef GINSENG_HPP
#define GINSENG_HPP

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Ginseng {

// Base IDs

using EID = ::std::int_fast64_t; // Entity ID
using CID = ::std::int_fast64_t; // Component ID
using TID = ::std::int_fast64_t; // Table ID

namespace _detail {

// Component Base

    class ComponentBase
    {
    public:
        virtual ~ComponentBase() = 0;
        virtual ::std::ostream& debugPrint(::std::ostream& out) const = 0;
        virtual ::std::unique_ptr<ComponentBase> clone() const = 0;
    };

    inline ComponentBase::~ComponentBase()
    {}

// Component CRTP

    template <typename Child>
    class Component
        : public _detail::ComponentBase
    {
        friend class Database;
        static TID tid;
      public:
        virtual ::std::ostream& debugPrint(::std::ostream& out) const override
        {
            out << "[ No debug message for TID " << tid << ". ]";
            return out;
        }

        virtual ::std::unique_ptr<_detail::ComponentBase> clone() const override
        {
            return ::std::unique_ptr<_detail::ComponentBase>(new Child(reinterpret_cast<const Child&>(*this)));
        }
    };

    template <typename Child>
    TID Component<Child>::tid = -1;

// Containers

    template <typename T, typename H = ::std::hash<T>>
    using Many = ::std::unordered_set<T, H>;

    template <typename K, typename V, typename H = ::std::hash<K>>
    using Table = ::std::unordered_map<K, V, H>;

// Types

    struct Types
    {
        template <typename T>
        struct EntityData
        {
            Table<TID, typename Table<CID,T>::iterator> coms;
        };

        template <typename T>
        class Entity
        {
            using Iter = typename Table<EID,EntityData<T>>::iterator;
            friend class Database;

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
            ComponentData(Entity<ComponentData> const& e, ::std::unique_ptr<ComponentBase>&& c);
        };

        struct ComponentTable
        {
            Table<CID,ComponentData> list;
            Many<Entity<ComponentData>, Entity<ComponentData>::Hash> ents;
        };
    };

    inline Types::ComponentData::ComponentData(Entity<ComponentData> const& e, ::std::unique_ptr<ComponentBase>&& c)
        : ent(new Entity<ComponentData>(e))
        , com(::std::move(c))
    {}

    using Entity         = Types::Entity<Types::ComponentData>;
    using EntityData     = Types::EntityData<Types::ComponentData>;
    using ComponentData  = Types::ComponentData;
    using ComponentTable = Types::ComponentTable;

// Query results

    template <typename... Ts>
    using RElement = ::std::tuple<Entity, Ts*...>;

    template <typename... Ts>
    using Result = ::std::vector<RElement<Ts...>>;

// Main Database engine

    class Database
    {
        Table<EID,EntityData> entities;
        Table<TID,ComponentTable> components;

        struct
        {
            EID eid = 0;
            TID tid = 0;
            CID cid = 0;
        } uidGen;

        EID createEntityID()
        {
            return ++uidGen.eid;
        }

        TID createTableID()
        {
            return ++uidGen.tid;
        }

        CID createCID()
        {
            return ++uidGen.cid;
        }

        template <int N = 2, typename T>
        typename ::std::enable_if<
            (N < ::std::tuple_size<T>::value) ,
        bool>::type fill_inspect(T& ele) const
        {
            using PtrType = typename ::std::tuple_element<N, T>::type;
            using Type = typename ::std::remove_pointer<PtrType>::type;

            auto const& coms = ::std::get<0>(ele).iter->second.coms;

            auto iter = coms.find(Type::tid);

            if (iter == coms.end())
            {
                return false;
            }

            ::std::get<N>(ele) = static_cast<PtrType>(iter->second->second.com.get());

            return fill_inspect<N+1>(ele);
        }

        template <int N = 2, typename T>
        typename ::std::enable_if<
            (N >= ::std::tuple_size<T>::value) ,
        bool>::type fill_inspect(T& ele) const
        {
            return true;
        }

        template <typename T, typename... Us>
        Result<T, Us...> select_inspect() const
        {
            Result<T, Us...> rval;

            const ComponentTable& table = components.at(T::tid);

            for (auto const& p : table.list)
            {
                const ComponentData& cd = p.second;
                T* data = static_cast<T*>(cd.com.get());

                RElement<T, Us...> ele;

                ::std::get<0>(ele) = *cd.ent;
                ::std::get<1>(ele) = data;

                if (fill_inspect(ele))
                {
                    rval.emplace_back(::std::move(ele));
                }
            }

            return rval;
        }

        template <int I = 0, typename... Ts>
        typename ::std::enable_if<
            I < ::std::tuple_size<::std::tuple<Ts*...>>::value,
        void>::type fill_components(::std::tuple<Ts*...>& tup, decltype(EntityData::coms) const& etab) const
        {
            using C = typename ::std::tuple_element<I, ::std::tuple<Ts...>>::type;
            TID tid = C::tid;

            auto iter = etab.find(tid);
            if (iter != etab.end())
            {
                ::std::get<I>(tup) = static_cast<C*>(iter->second->second.com.get());
            }

            return fill_components<I+1>(tup, etab);
        }

        template <int I = 0, typename... Ts>
        typename ::std::enable_if<
            I >= ::std::tuple_size<::std::tuple<Ts*...>>::value,
        void>::type fill_components(::std::tuple<Ts*...>& tup, decltype(EntityData::coms) const& etab) const
        {}

      public:

        enum class Selector
        {
            INSPECT
        };

        Entity newEntity()
        {
            EID eid = createEntityID();
            return entities.emplace(eid, EntityData{}).first;
        }

        Entity cloneEntity(Entity ent)
        {
            Entity rv = newEntity();

            for (const auto& p : ent.iter->second.coms)
            {
                auto tid = p.first;
                ComponentBase const* com = p.second->second.com.get();

                auto& table = components.at(tid);

                auto newcid = createCID();
                auto newcom = com->clone();

                ComponentData cd (rv, ::std::move(newcom));

                auto comiter = table.list.emplace(newcid, ::std::move(cd)).first;

                rv.iter->second.coms.emplace(tid, comiter);

                table.ents.insert(rv);
            }

            return rv;
        }

        void eraseEntity(Entity ent)
        {
            for (auto&& p : ent.iter->second.coms)
            {
                ComponentTable& tab = components.at(p.first);
                tab.list.erase(tab.list.find(p.second->first));
                tab.ents.erase(tab.ents.find(ent));
            }

            entities.erase(ent.iter);
        }

        template <typename T>
        TID registerComponent()
        {
            if (T::tid != -1) throw; //TODO
            T::tid = createTableID();
            return T::tid;
        }

        template <typename T, typename... Vs>
        T* newComponent(Entity ent, Vs&&... vs)
        {
            if (T::tid == -1) throw; //TODO

            CID cid = createCID();

            ::std::unique_ptr<_detail::ComponentBase> ptr (new T(::std::forward<Vs>(vs)...));
            T* rval = static_cast<T*>(ptr.get());

            ComponentTable& table = components[T::tid];
            ComponentData dat { ent, ::std::move(ptr) };

            auto comiter = table.list.emplace(cid, ::std::move(dat)).first;
            table.ents.insert(ent);

            ent.iter->second.coms.emplace(T::tid, comiter);

            return rval;
        }

        template <typename... Ts>
        Result<Ts...> getEntities(const Selector method = Selector::INSPECT) const
        {
            switch (method)
            {
                case Selector::INSPECT:
                    return select_inspect<Ts...>();
            }

            throw; // TODO
        }

        template <typename... Ts>
        ::std::tuple<Ts*...> getComponents(Entity ent) const
        {
            ::std::tuple<Ts*...> rv;

            fill_components<0>(rv, ent.iter->second.coms);

            return rv;
        }

        decltype(entities.size()) numEntities() const
        {
            return entities.size();
        }

        ::std::ostream& debugPrint(::std::ostream& out) const
        {
            out << "Ginseng::Database::debugPrint() unimplemented!" << ::std::endl;

#if 0
            out << "Entity count: " << entities.size() << ::std::endl;
            out << "Entity total: " << uidGen.eid << ::std::endl;
            for (auto&& p : entities)
            {
                EID eid = p.first;
                auto&& cids = p.second;

                out << ::std::setw(3) << eid;
                for (auto&& cid : cids)
                {
                    out << ' ';
                    out << ::std::setw(3) << cid.first;
                    out << ':';
                    out << ::std::setw(2) << ::std::left << cid.second;
                    out << ::std::right;
                }
                out << ::std::endl;
            }

            out << "Components registered: " << uidGen.tid << ::std::endl;
            out << "Components used:       " << components.size() << ::std::endl;
            for (auto&& p : components)
            {
                TID tid = p.first;
                const ComponentTable& table = p.second;
                const auto& list = table.list;
                const Many<EID>& eidlist = table.eids;

                ::std::vector<EID> eids (begin(eidlist), end(eidlist));
                ::std::sort(begin(eids), end(eids));

                out << "Component " << tid << ":" << ::std::endl;
                out << "    Entities:";
                for (auto&& eid : eids) out << ' ' << ::std::setw(5) << eid;
                out << ::std::endl;
                for (auto&& q : list)
                {
                    CID cid = q.first;
                    const ComponentData& data = q.second;
                    out << "    ";
                    out << ::std::setw(5) << ::std::left << cid;
                    out << ::std::right;
                    out << ' ' << ::std::setw(5) << data.eid;
                    out << ' ';
                    data.com->debugPrint(out);
                    out << ::std::endl;
                }
            }
#endif

            return out;
        }
};

} // namespace _detail

// Public types

template <typename Child>
using Component = _detail::Component<Child>;

using Entity = _detail::Entity;
using Database = _detail::Database;

} // namespace Ginseng

#endif
