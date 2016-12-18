//
// Created by lz on 11/2/16.
//

#ifndef GO_AI_POS_GROUP_HPP
#define GO_AI_POS_GROUP_HPP

#include <cstddef>
#include <cstdint>
#include <list>
#include <algorithm>
#include <iterator>
#include <functional>
#include <array>
#include <vector>
#include "grid_point.hpp"
#include "group_node.hpp"
#include "logger.hpp"

namespace board
{
    template <std::uint_least8_t W, std::uint_least8_t H>
    class PosGroup
    {
        using GroupIterator = typename std::list< GroupNode<W, H> >::iterator;
        using GroupConstIterator = typename std::list< GroupNode<W, H> >::const_iterator;
        std::shared_ptr<spdlog::logger> logger = getGlobalLogger();
    public:
        using PointType = GridPoint<W, H>;

        struct GroupConstIteratorHash
        {
            std::hash<const GroupNode<W, H>*> h;
            std::size_t operator() (GroupConstIterator gci) const
            {
                return h(&(*gci));
            }
        };

        struct ItemType
        {
            union Value
            {
                GroupIterator groupIterator;
                PointType pointType;

                Value(GroupIterator gi): groupIterator(gi) {}
                Value(PointType p): pointType(p) {}
                Value() {}
            } value;
            enum struct Type {GroupIterator, PointType} type;
            ItemType()
            {
            }
        };
    protected:
        mutable std::array<ItemType, W * H> arr;

        static std::size_t pointToIndex(PointType p)
        {
            return p.x * W + p.y;
        }

        PointType findfa(PointType p) const
        {
            std::size_t idx = pointToIndex(p);
            if (arr[idx].type == ItemType::Type::GroupIterator)
            {
                return p;
            }
            else
            {
                arr[idx].value.pointType = findfa(arr[idx].value.pointType);
                return arr[idx].value.pointType;
            }
        }

    public:
        PosGroup() = default;
        PosGroup(const PosGroup& other,
                 const std::vector<std::pair<GroupConstIterator , GroupIterator>> &oldToNewMap):
                arr(other.arr), logger(other.logger)
        {
            PointType::for_all([&](PointType p) {
                if (arr[pointToIndex(p)].type == ItemType::Type::GroupIterator)
                {
                    using PairT = std::pair<GroupConstIterator, GroupConstIterator>;
                    auto it = std::find_if(oldToNewMap.cbegin(), oldToNewMap.cend(), [&](const PairT &pair) {
                        return pair.first == arr[pointToIndex(p)].value.groupIterator;
                    });
                    arr[pointToIndex(p)].value.groupIterator = it->second;
                }
            });
        }
        PosGroup(GroupIterator default_it)
        {
            fill(default_it);
        }
        void fill(GroupIterator default_it)
        {
            std::for_each(std::begin(arr), std::end(arr), [&](ItemType &item) {
                item.type = ItemType::Type::GroupIterator;
                item.value.groupIterator = default_it;
            });
        }
        GroupIterator get(PointType p) const
        {
            PointType fa = findfa(p);
            return arr[pointToIndex(fa)].value.groupIterator;
        }
        // Use this only if this is the first time set(*, this_iterator) is called! otherwise please use merge()!
        void set(PointType p, GroupIterator it)
        {
            arr[pointToIndex(p)].type = ItemType::Type::GroupIterator;
            arr[pointToIndex(p)].value.groupIterator = it;
        }

        // Merge p2 into p1
        // fa[ getfa(p2) ] = getfa(p1)
        void merge(PointType p1, PointType p2)
        {
            PointType fa1 = findfa(p1), fa2 = findfa(p2);
            if (fa1 != fa2) {
                arr[pointToIndex(fa2)].type = ItemType::Type::PointType;
                arr[pointToIndex(fa2)].value.pointType = p1;
            }
        }

    };
}
#endif //GO_AI_POS_GROUP_HPP
