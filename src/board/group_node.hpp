//
// Created by lz on 11/2/16.
//

#ifndef GO_AI_GROUP_NODE_HPP
#define GO_AI_GROUP_NODE_HPP

#include <climits>
#include <cstdint>
#include <compressed_grid.hpp>
#include "basic.hpp"

namespace board
{
    template<std::size_t W, std::size_t H>
    struct GroupNode
    {
    public:
        using CGType = compgrid::CompressedGrid<bool, W, H, 1>;
        using PointType = typename CGType::PointType;
    private:
        Player player;
        CGType liberty_grid;
        std::size_t stone_cnt;
    public:
        GroupNode() = default;
        explicit GroupNode(Player p, std::size_t stone_cnt = 1, const CGType &cg = CGType()):
                player(p), liberty_grid(cg), stone_cnt(stone_cnt)
        {}
        std::size_t getLiberty() const
        {
            return liberty_grid.count();
        }
        // set a position is/is not controlled by this group
        void setLiberty(typename CGType::PointType point, bool value)
        {
            liberty_grid.set(point, value);
        }
        void addLiberty(int libDelta)
        {
            libDelta += libDelta;
        }
        Player getPlayer() const
        {
            return static_cast<Player>(static_cast<unsigned>(player) & 1);
        }
        void setPlayer(Player p)
        {
            player = p;
        }

        void merge(const GroupNode &other)
        {
            if (&other != this)
            {
                liberty_grid |= other.liberty_grid;
                stone_cnt += other.stone_cnt;
            }
        }

        std::size_t getStoneCnt() const
        {
            return stone_cnt;
        }
    };
}
#endif //GO_AI_GROUP_NODE_HPP
