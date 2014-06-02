#ifndef TYPES_HPP
#define TYPES_HPP

#include "ginseng/ginseng.hpp"
#include "puddle/puddle.hpp"

template <typename T>
using PoolAllocator = Puddle::Allocator<T>;

using ECDatabase = Ginseng::Database<PoolAllocator>;

using Ginseng::Not;

using EntID = ECDatabase::EntID;
using ComID = ECDatabase::ComID;

#endif // TYPES_HPP
