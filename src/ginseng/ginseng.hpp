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

// Private Namespace

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

// Private Types

struct Types
{
    // Templates

    template <typename T, typename H = ::std::hash<T>>
    using Many = ::std::unordered_set<T, H>;

    template <typename K, typename V, typename H = ::std::hash<K>>
    using Table = ::std::unordered_map<K, V, H>;

    template <typename... Ts>
    using RElement = ::std::tuple<EID, Ts*...>;

    template <typename... Ts>
    using Result = ::std::vector<RElement<Ts...>>;

    // Aggregate IDs

    struct CTID
    {
        CID cid;
        TID tid;
    };

    struct ComponentData
    {
        EID eid;
        ::std::unique_ptr<ComponentBase> com;
    };

    struct ComponentTable
    {
        Table<CID,ComponentData> list;
        Many<EID> eids;
    };

    // Custom Hashes

    struct CTIDHash
    {
        ::std::size_t operator()(const CTID& ctid) const
        {
            return ::std::hash<CID>{}(ctid.cid);
        }
    };
};

} // namespace _detail

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

// Main Database engine

class Database final
    : private _detail::Types
{
    Table<EID,Table<TID,CID>> entities;
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

    template <typename T>
    T* translateCID(CID cid) const
    {
        return static_cast<T*>(components.at(T::tid).list.at(cid).com.get());
    }

    template <int N = 2, typename T>
    typename ::std::enable_if<
        (N >= ::std::tuple_size<T>::value) ,
    bool>::type fill_inspect(T& ele) const
    {
        return true;
    }

    template <int N = 2, typename T>
    typename ::std::enable_if<
        (N < ::std::tuple_size<T>::value) ,
    bool>::type fill_inspect(T& ele) const
    {
        using PtrType = typename ::std::tuple_element<N, T>::type;
        using Type = typename ::std::remove_pointer<PtrType>::type;

        EID eid = ::std::get<0>(ele);
        const Table<TID,CID>& tcids = entities.at(eid);

        auto iter = tcids.find(Type::tid);

        if (iter == end(tcids)) return false;

        ::std::get<N>(ele) = translateCID<Type>(iter->second);

        return fill_inspect<N+1>(ele);
    }

    template <typename T, typename... Us>
    Result<T, Us...> select_inspect() const
    {
        Result<T, Us...> rval;

        const ComponentTable& table = components.at(T::tid);
        const auto& list = table.list;

        for (auto&& p : list)
        {
            const ComponentData& cd = p.second;
            EID eid = cd.eid;
            T* data = static_cast<T*>(cd.com.get());

            RElement<T, Us...> ele;

            ::std::get<0>(ele) = eid;
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
    void>::type fill_components(::std::tuple<Ts*...>& tup, Table<TID,CID> const& etab) const
    {
        using C = typename ::std::tuple_element<I, ::std::tuple<Ts...>>::type;
        TID tid = C::tid;

        auto iter = etab.find(tid);
        if (iter != etab.end())
        {
            CID cid = iter->second;
            ::std::get<I>(tup) = translateCID<C>(cid);
        }

        return fill_components<I+1>(tup, etab);
    }

    template <int I = 0, typename... Ts>
    typename ::std::enable_if<
        I >= ::std::tuple_size<::std::tuple<Ts*...>>::value,
    void>::type fill_components(::std::tuple<Ts*...>& tup, Table<TID,CID> const& etab) const
    {}

  public:

    enum class Selector
    {
        INSPECT
    };

    EID newEntity()
    {
        return createEntityID();
    }

    EID cloneEntity(EID ent)
    {
        auto rv = newEntity();

        auto iter = entities.find(ent);
        auto enttab = entities.end();

        if (iter != entities.end())
        {
            for (const auto& p : iter->second)
            {
                TID tid = p.first;
                CID cid = p.second;

                auto& table = components.at(tid);

                auto newcid = createCID();
                ComponentData cd {rv, table.list.at(cid).com->clone()};
                table.list.emplace(newcid, ::std::move(cd));

                if (enttab == entities.end()) enttab = entities.emplace(rv, Table<TID,CID>{}).first;
                enttab->second[tid] = newcid;
            }
        }

        return rv;
    }

    void eraseEntity(EID ent)
    {
        auto eiter = entities.find(ent);

        if (eiter == end(entities)) throw; // TODO

        Table<TID,CID>& comps = eiter->second;

        for (auto&& p : comps)
        {
            ComponentTable& tab = components.at(p.first);
            auto& list = tab.list;
            Many<EID>& eids = tab.eids;

            {
                auto citer = list.find(p.second);
                list.erase(citer);
            }

            {
                auto liter = eids.find(ent);
                eids.erase(liter);
            }
        }

        entities.erase(eiter);
    }

    template <typename T>
    TID registerComponent()
    {
        if (T::tid != -1) throw; //TODO
        T::tid = createTableID();
        return T::tid;
    }

    template <typename T, typename... Vs>
    T* newComponent(EID ent, Vs&&... vs)
    {
        if (T::tid == -1) throw; //TODO

        CID cid = createCID();

        ::std::unique_ptr<_detail::ComponentBase> ptr (new T(::std::forward<Vs>(vs)...));
        T* rval = static_cast<T*>(ptr.get());

        entities[ent][T::tid] = cid;

        ComponentTable& table = components[T::tid];
        {
            auto& list = table.list;
            {
                ComponentData dat { ent, ::std::move(ptr) };
                list.insert(::std::make_pair(cid, ::std::move(dat)));
            }
        }
        {
            Many<EID>& eset = table.eids;
            eset.insert(ent);
        }

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
    ::std::tuple<Ts*...> getComponents(EID eid) const
    {
        ::std::tuple<Ts*...> rv;

        auto iter = entities.find(eid);
        if (iter != entities.end())
        {
            fill_components<0>(rv, iter->second);
        }

        return rv;
    }

    decltype(entities.size()) numEntities() const
    {
        return entities.size();
    }

    ::std::ostream& debugPrint(::std::ostream& out) const
    {
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

        return out;
    }
};

} // namespace Ginseng

#endif
