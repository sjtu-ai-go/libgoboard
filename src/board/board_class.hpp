//
// Created by lz on 11/2/16.
//

#include <cstddef>
#include <memory>
#include <functional>
#include <list>

#ifndef GO_AI_BOARD_CLASS_HPP
#define GO_AI_BOARD_CLASS_HPP

#include "basic.hpp"
#include "grid_point.hpp"
#include "group_node.hpp"
#include "pos_group.hpp"
#include "board_grid.hpp"
#include <ostream>
#include <vector>
#include <cassert>
#include <memory>
#include <map>
#include <queue>
#include <logger.hpp>
#include <spdlog/fmt/ostr.h>
#include "message.pb.h"

namespace board
{
    template<std::size_t W, std::size_t H>
    class Board;
    template<std::size_t W, std::size_t H>
    std::ostream &  operator<<(std::ostream & o, const Board<W, H> & b);
}

namespace std
{
    template<std::size_t W, std::size_t H>
    struct hash<board::Board<W, H>>;
}

namespace board
{
    template <std::size_t W, std::size_t H>
    class Board
    {
    private:

        static const std::size_t INIT_LASTSTATEHASH = 0x24512211u,
            INIT_CURSTATEHASH = 0xc7151360u;
        static const std::size_t MAX_HISTORY_LENGTH = 7;
        std::shared_ptr<spdlog::logger> logger = getGlobalLogger();
        BoardGrid<W, H> boardGrid_;
        std::list< GroupNode<W, H> > groupNodeList_;
        PosGroup<W, H> posGroup_ = {groupNodeList_.end()};
        std::size_t step_ = 0;
        std::size_t lastStateHash_ = INIT_LASTSTATEHASH; // The hash of board 1 steps before. Used to validate ko.
        std::size_t curStateHash_ = INIT_CURSTATEHASH; // Hash of current board
        using PosGroupType = PosGroup<W, H>;

    public:
        using PointType = GridPoint<W, H>;
        using GroupNodeType = GroupNode<W, H>;
        using GroupListType = std::list< GroupNodeType >;
        using GroupIterator = typename GroupListType::iterator;
        using GroupConstIterator = typename GroupListType::const_iterator;
        friend class std::hash<Board>;
        static const std::size_t w = W;
        static const std::size_t h = H;
    private:
        std::queue<PointType> placeHistory_;
        PointType lastMovePoint = {0, 0};
        PointType koPoint = {-1, -1}; // -1, -1 if none
        Player koPlayer = Player::B;

        std::vector< std::pair<GroupConstIterator, GroupIterator> >
        getMapFromOldItToNewIt(GroupListType &newList,
                               const GroupListType &oldList)
        {
            assert(newList.size() == oldList.size());
            std::vector< std::pair<GroupConstIterator, GroupIterator>> vmap;
            vmap.reserve(32);

            auto it_new = newList.begin();
            auto it_old = oldList.cbegin();
            vmap.push_back(std::make_pair(oldList.cend(), newList.end()));
            for(; it_new != newList.end() && it_old != oldList.end(); ++it_new, ++it_old)
            {
                vmap.push_back(std::make_pair(it_old, it_new));
            }
            return vmap;
        };
    public:

        Board()
        {
        }

        Board(const Board &other):
                boardGrid_(other.boardGrid_),
                groupNodeList_(other.groupNodeList_),
                posGroup_(other.posGroup_, getMapFromOldItToNewIt(groupNodeList_, other.groupNodeList_)),
                placeHistory_(other.placeHistory_),
                lastStateHash_(other.lastStateHash_),
                curStateHash_(other.curStateHash_),
                step_(other.step_),
                lastMovePoint(other.lastMovePoint),
                koPoint(other.koPoint),
                koPlayer(other.koPlayer)
        {
        }

        Board& operator=(const Board &other)
        {
            if (this != &other)
            {
                boardGrid_ = other.boardGrid_;
                groupNodeList_ = other.groupNodeList_;
                posGroup_ = decltype(posGroup_)
                            (other.posGroup_, getMapFromOldItToNewIt(groupNodeList_, other.groupNodeList_));
                placeHistory_ = other.placeHistory_;
                lastStateHash_ = other.lastStateHash_;
                curStateHash_ = other.curStateHash_;
                step_ = other.step_;
                lastMovePoint = other.lastMovePoint;
                koPoint = other.koPoint;
                koPlayer = other.koPlayer;
            }
            return *this;
        }

        void clear()
        {
            groupNodeList_.clear();
            posGroup_.fill(groupNodeList_.end());

            std::queue<PointType> empty;
            std::swap(placeHistory_, empty);

            boardGrid_.clear();
            lastStateHash_ = INIT_LASTSTATEHASH;
            curStateHash_ = INIT_CURSTATEHASH;
            step_ = 0;
            lastMovePoint.x = 0; lastMovePoint.y = 0;
        }

        // Returns color of a point
        PointState getPointState(PointType p) const
        {
            return boardGrid_.get(p);
        }
        // Returns pointer to group of a point. NULL if there is no piece
        GroupConstIterator getPointGroup(PointType p) const
        {
            return posGroup_.get(p);
        }
        // Return a copy of placeHistory_
        std::queue<PointType> getHistoryCopy() const {
            return placeHistory_;
        }
        std::size_t getStep() const
        {
            return step_;
        }
        // place a piece on the board. State will be changed
        void place(PointType p, Player player);

        enum struct PositionStatus
        {
            OK, // Legal to place a piece
            NOCHANGE, // The piece to place is immediately taken away, and no change does it make to board
            KO, // Violates the rule of Ko
            NOTEMPTY, // The place is not empty
            SUICIDE // The piece doesn't survive after place
        };
        // Whether it is legal/why it is illegal to place a piece of player at p. State will not be changed.
        PositionStatus getPosStatus(PointType p, Player player);
        // Find all valid position for player
        std::vector<PointType> getAllValidPosition(Player player)
        {
            std::vector<PointType> ans;
            for (std::size_t i=0; i<W; ++i)
                for (std::size_t j=0; j<H; ++j)
                {
                    bool temp_disable_log = (int)logger->level() == (int)spdlog::level::trace;
                    if (temp_disable_log)
                        logger->set_level(spdlog::level::debug);
                    PositionStatus status = getPosStatus(PointType(i, j), player);
                    if (temp_disable_log)
                        logger->set_level(spdlog::level::trace);
                    if (status == PositionStatus::OK) {
                        ans.push_back(PointType(i, j));
                    } else {
                        logger->trace("Illegal move at ({}, {}): state: {}", i, j, (int)status);
                    }
                }
            return ans;
        }
        // Returns first node(may be empty) of GroupNode link list
        GroupConstIterator groupBegin() const
        {
            return groupNodeList_.cbegin();
        }

        GroupConstIterator groupEnd() const
        {
            return groupNodeList_.cend();
        }

        bool isEye(PointType p, Player player);
        bool isSemiEye(PointType p, Player player);
        bool isFakeEye(PointType p, Player player);
        bool isTrueEye(PointType p, Player player);
        bool isSelfAtari(PointType p, Player player);
        PointType getSimpleKoPoint() const; // Returns simple ko point. (-1, -1) when no simple ko
        std::vector<PointType> getAllGoodPosition(Player player);

        friend std::ostream& operator<< <>(std::ostream&, const Board&);

        gocnn::RequestV1 generateRequestV1(Player player);
        gocnn::RequestV2 generateRequestV2(Player player);
        gocnn::RequestV2 generateRequestV2Bug(Player player); // Bug workaround version

    private:
        // Internal use only
        GroupIterator getPointGroup_(PointType p)
        {
            return posGroup_.get(p);
        }
        PositionStatus getPosStatusAndPlace(PointType p, Player player);
        void removeGroup(GroupIterator group);
        void removeGroupFromPos(PointType p);
        void mergeGroupAt(PointType thisPoint, PointType otherPoint);
        std::vector<GroupIterator> getAdjacentGroups(PointType p);
        static inline bool GroupIteratorLess(const GroupIterator& it1, const GroupIterator& it2)
        {
            return &(*it1) < &(*it2);
        }
    };

    template<std::size_t W, std::size_t H>
    auto Board<W, H>::getAdjacentGroups(PointType p) -> std::vector<GroupIterator>
    {
        std::vector<GroupIterator> adjGroups;
        adjGroups.reserve(4);
        p.for_each_adjacent([&](PointType adjP) {
            GroupIterator group = getPointGroup_(adjP);
            if (group != groupNodeList_.end())
                adjGroups.push_back(group);
        });
        // remove Duplicated groups
        std::sort(adjGroups.begin(), adjGroups.end(), Board::GroupIteratorLess);
        auto newEnd = std::unique(adjGroups.begin(), adjGroups.end());
        adjGroups.erase(newEnd, adjGroups.end());
        return adjGroups;
    }

    template<std::size_t W, std::size_t H>
    void Board<W, H>::removeGroup(GroupIterator group)
    {
        std::vector<PointType> point_to_remove;
        point_to_remove.reserve(W * H);
        PointType::for_all([&](PointType p) {
            if (getPointGroup(p) == group)
            {
                std::vector<GroupIterator> adjGroups = getAdjacentGroups(p);
                std::for_each(adjGroups.begin(), adjGroups.end(), [&](GroupIterator adjGroup) {
                    if (adjGroup != group) adjGroup->setLiberty(p, true);
                });
                boardGrid_.set(p, PointState::NA);
                // posGroup_.set(p, groupNodeList_.end());
                // cannot delete here, since union-set would stuck into inconsistent state
                point_to_remove.push_back(p);
            }
        });
        std::for_each(point_to_remove.begin(), point_to_remove.end(), [&](PointType p){
            posGroup_.set(p, groupNodeList_.end());
        });
        groupNodeList_.erase(group);
    }

    template<std::size_t W, std::size_t H>
    void Board<W, H>::removeGroupFromPos(PointType p)
    {
        GroupIterator group = getPointGroup_(p);
        std::vector<PointType> point_to_remove;
        point_to_remove.reserve(W * H);

        std::queue<PointType> point_to_visit;
        bool visited[W * H] {};
        point_to_visit.push(p);
        while (!point_to_visit.empty())
        {
            PointType p = point_to_visit.front();
            point_to_visit.pop();

            p.for_each_adjacent([&](PointType adjP) {
                GroupIterator adjGroup = getPointGroup_(adjP);
                if (adjGroup == group)
                {
                    if (!visited[adjP.x * H + adjP.y])
                    {
                        point_to_visit.push(adjP);
                        visited[adjP.x * H + adjP.y] = true;
                    }
                } else if (adjGroup != groupNodeList_.end())
                {
                    adjGroup->setLiberty(p, true);
                }
            });

            boardGrid_.set(p, PointState::NA);
            // posGroup_.set(p, groupNodeList_.end());
            // cannot delete here, since union-set would stuck into inconsistent state
            point_to_remove.push_back(p);
        }
        std::for_each(point_to_remove.begin(), point_to_remove.end(), [&](PointType p){
            posGroup_.set(p, groupNodeList_.end());
        });
        groupNodeList_.erase(group);
    }

    template<std::size_t W, std::size_t H>
    void Board<W, H>::mergeGroupAt(PointType thisPoint, PointType thatPoint)
    {
        GroupIterator thisGroup = getPointGroup_(thisPoint), thatGroup = getPointGroup_(thatPoint);
        posGroup_.merge(thisPoint, thatPoint);

        thisGroup->merge(*thatGroup);
        groupNodeList_.erase(thatGroup);
    }

    template<std::size_t W, std::size_t H>
    void Board<W,H>::place(PointType p, Player player)
    {
        logger->trace("Place at {}, {}: {}", (int)p.x, (int)p.y, (int) player);
        if (getPointState(p) != PointState::NA)
            throw std::runtime_error("Try to place on an non-empty point");

        boardGrid_.set(p, getPointStateFromPlayer(player));

        Player opponent = getOpponentPlayer(player);

        // --- Decrease liberty of adjacent groups
        std::vector<GroupIterator> adjGroups = getAdjacentGroups(p);
        std::for_each(adjGroups.begin(), adjGroups.end(), [&](GroupIterator group) {
            logger->trace("Adjacent groups's liberty: {}", group->getLiberty());
        });

        std::size_t removed_stones = 0;
        PointType last_removed_point {-1, -1};
        // Update liberty and remove opponent's dead groups (liberty of our group may change)
        p.for_each_adjacent([&](PointType adjP) {
            GroupIterator group = getPointGroup_(adjP);
            if (group != groupEnd())
            {
                group->setLiberty(p, false);
                if (group->getPlayer() == opponent && group->getLiberty() == 0)
                {
                    removed_stones += group->getStoneCnt();
                    logger->trace("Removing group with liberty {}", group->getLiberty());
                    last_removed_point = adjP;
                    removeGroupFromPos(adjP);
                }
            }
        });

        // --- Add this group
        logger->trace("Adding this group");

        GroupNodeType gn(player, 1);
        p.for_each_adjacent([&](PointType adjP) {
            logger->trace("Adjacent point {},{} is empty, setting liberty", (int)adjP.x, (int)adjP.y);
            if (getPointState(adjP) == PointState::NA)
                gn.setLiberty(adjP, true);
            logger->trace("Current liberty: {}", gn.getLiberty());
        });
        auto thisGroup = groupNodeList_.insert(groupNodeList_.cbegin(), gn);
        posGroup_.set(p, thisGroup);
        logger->trace("After set: {}", *this);

        // --- Merge our group
        logger->trace("Merging group");
        p.for_each_adjacent([&](PointType adjP) {
            GroupIterator adjPointGroup = getPointGroup_(adjP);
            if (adjPointGroup != groupNodeList_.end() &&
                    adjPointGroup->getPlayer() == player &&
                    adjPointGroup != thisGroup)
            {
                logger->trace("Merging group with liberty {}", adjPointGroup->getLiberty());
                mergeGroupAt(p, adjP);
            }
        });

        if (thisGroup->getStoneCnt() == 1 && thisGroup->getLiberty() == 1 && removed_stones == 1)
        {
            koPoint = last_removed_point;
            koPlayer = opponent;
        }
        else
            koPoint = PointType(-1, -1);

        // --- remove our dead groups
        if (thisGroup->getLiberty() == 0) {
            removeGroup(thisGroup);
            logger->trace("Removing self...");
        }
        p.for_each_adjacent([&](PointType p) {
            GroupIterator adjPointGroup = getPointGroup_(p);
            if (adjPointGroup != groupNodeList_.end() && adjPointGroup->getLiberty() == 0) {
                logger->trace("Removing group at ({}, {})", (int)p.x, (int) p.y);
            }
        });
        logger->trace("After move:{}", *this);
        std::hash<Board> h;
        std::size_t hash_v = h(*this);
        logger->trace("last 2 hash: {}, last 1 hash: {}, cur Hash: {}", lastStateHash_, curStateHash_, hash_v);
        lastStateHash_ = curStateHash_;
        curStateHash_ = hash_v;
        lastMovePoint = p;
        ++step_;
        if (placeHistory_.size() >= MAX_HISTORY_LENGTH)
            placeHistory_.pop();
        placeHistory_.push(p);
    }

    template<std::size_t W, std::size_t H>
    auto Board<W,H>::getPosStatus(PointType p, Player player) -> typename Board::PositionStatus
    {
        if (getPointState(p) != PointState::NA)
            return PositionStatus::NOTEMPTY;

        if (p == getSimpleKoPoint() && player == koPlayer)
            return PositionStatus::KO;

        int our_group_liberty_greater_than_1 = 0, oppo_group_liberty_1 = 0;
        bool has_free = false;
        p.for_each_adjacent([&](PointType adjP) {
            if (!has_free) {
                auto group = getPointGroup_(adjP);
                if (group != groupEnd()) {
                    if (group->getPlayer() == player && group->getLiberty() > 1)
                        ++our_group_liberty_greater_than_1;
                    if (group->getPlayer() != player && group->getLiberty() == 1)
                        ++oppo_group_liberty_1;
                }
                else
                {
                    has_free = true;
                }
            }
        });
        if (!has_free && our_group_liberty_greater_than_1 == 0 && oppo_group_liberty_1 == 0)
            return PositionStatus::SUICIDE;

        return PositionStatus::OK;
    };

    template<std::size_t W, std::size_t H>
    auto Board<W, H>::getPosStatusAndPlace(PointType p, Player player) -> typename Board::PositionStatus
    {
        if (getPointState(p) != PointState::NA)
            return Board::PositionStatus::NOTEMPTY;
        Board &testBoard = *this;
        logger->trace("After copy: {}", testBoard);

        std::size_t last2hash = testBoard.lastStateHash_;
        testBoard.place(p, player);
        if (testBoard.lastStateHash_ == testBoard.curStateHash_)
            return Board::PositionStatus::NOCHANGE;
        if (testBoard.curStateHash_ == last2hash)
            return Board::PositionStatus::KO;
        if (testBoard.getPointState(p) == PointState::NA)
            return Board::PositionStatus::SUICIDE;
        return Board::PositionStatus::OK;
    }

    template<std::size_t W, std::size_t H>
    bool Board<W, H>::isEye(PointType p, Player player)
    {
        if (getPointState(p) != PointState::NA)
            return false;
        bool isEye = true;
        p.for_each_adjacent([&](PointType adjP){
            if (getPointState(adjP) != getPointStateFromPlayer(player))
                isEye = false;
        });
        return isEye;
    }

    template<std::size_t W, std::size_t H>
    bool Board<W, H>::isSemiEye(PointType p, Player player)
    {
        if (!isEye(p, player))
            return false;
        std::size_t oppo_cnt = 0, empty_cnt = 0, all_cnt = 0;
        p.for_each_diag([&](PointType adjP) {
            PointState ps = getPointState(adjP);
            ++all_cnt;
            if (ps == PointState::NA) {
                if (!isEye(adjP, player))
                    ++empty_cnt;
            } else
                if (ps != getPointStateFromPlayer(player))
                    ++oppo_cnt;
        });
        return (all_cnt == 4 && oppo_cnt == 1 && empty_cnt == 1) ||
                (all_cnt < 4 && oppo_cnt == 0 && empty_cnt == 1);
    }

    template<std::size_t W, std::size_t H>
    bool Board<W, H>::isFakeEye(PointType p, Player player)
    {
        std::size_t oppo_cnt = 0, all_cnt = 0;
        p.for_each_diag([&](PointType adjP) {
            ++all_cnt;
            PointState ps = getPointState(adjP);
            if (ps != PointState::NA && ps != getPointStateFromPlayer(player))
                ++oppo_cnt;
        });
        return (all_cnt < 4 && oppo_cnt >= 1) ||
                (all_cnt == 4 && oppo_cnt >=2);
    }

    template<std::size_t W, std::size_t H>
    bool Board<W, H>::isTrueEye(PointType p, Player player)
    {
        return isEye(p, player) && !isFakeEye(p, player);
    }

    template<std::size_t W, std::size_t H>
    bool Board<W, H>::isSelfAtari(PointType p, Player player)
    {
        if (getPointState(p) != PointState::NA)
            return false;

        int liberty = 4;

        if (p.x == 0 || p.x == H - 1)
            --liberty;
        if (p.y == 0 || p.y == W - 1)
            --liberty;

        std::vector<GroupConstIterator> groupList;
        groupList.reserve(4);

        bool captureOpponent = false;

        p.for_each_adjacent([&](PointType adjP) {
            GroupConstIterator adjGroup = getPointGroup(adjP);
            if (getPointState(adjP) == getPointStateFromPlayer(getOpponentPlayer(player)))
            {
                if (adjGroup->getLiberty() <= 1)
                    captureOpponent = true;
                --liberty;
            }
            else if (getPointState(adjP) == getPointStateFromPlayer(player))
                groupList.push_back(adjGroup);
        });

        if (captureOpponent)
            return false;

        for (typename std::vector<GroupConstIterator>::iterator iter = groupList.begin(); iter != groupList.end(); ++iter)
        {
            bool duplicated = false;
            for (typename std::vector<GroupConstIterator>::iterator iter2 = groupList.begin(); iter2 != iter; ++iter2)
                if (*iter == *iter2)
                {
                    liberty -= 2;
                    duplicated = true;
                }

            if (!duplicated)
                liberty += (*iter)->getLiberty() - 2;
        }

        if ((!p.is_left() && !p.is_top()) &&
                (getPointState(p.left_up_point()) == PointState::NA) &&
                (getPointState(p.left_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointState(p.up_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointGroup(p.left_point()) != getPointGroup(p.up_point())))
            --liberty;

        if ((!p.is_left() && !p.is_bottom()) &&
                (getPointState(p.left_down_point()) == PointState::NA) &&
                (getPointState(p.left_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointState(p.down_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointGroup(p.left_point()) != getPointGroup(p.down_point())))
            --liberty;

        if ((!p.is_right() && !p.is_top()) &&
                (getPointState(p.right_up_point()) == PointState::NA) &&
                (getPointState(p.right_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointState(p.up_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointGroup(p.right_point()) != getPointGroup(p.up_point())))
            --liberty;

        if ((!p.is_right() && !p.is_bottom()) &&
                (getPointState(p.right_down_point()) == PointState::NA) &&
                (getPointState(p.right_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointState(p.down_point()) == getPointStateFromPlayer(getOpponentPlayer(player))) &&
                (getPointGroup(p.right_point()) != getPointGroup(p.down_point())))
            --liberty;

        if (liberty >= 2)
            return false;

        return true;
    }

    template<std::size_t W, std::size_t H>
    auto Board<W, H>::getAllGoodPosition(Player player) -> std::vector<PointType>
    {
        auto validPos = getAllValidPosition(player);
        validPos.erase(std::remove_if(validPos.begin(), validPos.end(), [&](PointType p) {
            return isTrueEye(p, player) || isSelfAtari(p, player);
        }), validPos.end());
        return validPos;
    }

    template<std::size_t W, std::size_t H>
    auto Board<W, H>::getSimpleKoPoint() const -> PointType
    {
        return koPoint;
    }

    template<std::size_t W, std::size_t H>
    std::ostream &  operator<<(std::ostream & o, const Board<W, H> & b) {
        using PT = typename Board<W, H>::PointType;
        o << "Points" << std::endl;


        for (int j=H-1; j>=0; --j)
        {
            o << j + 1 << '\t';
            for (int i=0; i<W; ++i)
                o << (int) b.boardGrid_.get(PT {(char)i, (char)j}) << ' ';
            o << std::endl;
        }
        o << "Group Liberties"<< std::endl;
        for (int j=H-1; j>=0; --j)
        {
            o << j + 1 << '\t';
            for (int i=0; i<W; ++i) {
                PT p {(char)i, (char)j};
                auto group = b.getPointGroup(p);
                if (group == b.groupNodeList_.end())
                    o << "O\t";
                else
                    o << group->getLiberty() << "\t";
            }
            o << std::endl;
        }
        return o;
    }

    template<std::size_t W, std::size_t H>
    auto Board<W, H>::generateRequestV1(Player player) -> gocnn::RequestV1
    {
        gocnn::RequestV1 reqv1;
        reqv1.set_board_size(W * H);
        reqv1.mutable_is_simple_ko()->Reserve(reqv1.board_size());
        reqv1.mutable_our_group_lib1()->Reserve(reqv1.board_size());
        reqv1.mutable_our_group_lib2()->Reserve(reqv1.board_size());
        reqv1.mutable_our_group_lib3_plus()->Reserve(reqv1.board_size());
        reqv1.mutable_oppo_group_lib1()->Reserve(reqv1.board_size());
        reqv1.mutable_oppo_group_lib2()->Reserve(reqv1.board_size());
        reqv1.mutable_oppo_group_lib3_plus()->Reserve(reqv1.board_size());
        PointType::for_all([&](PointType p) {
            auto group = getPointGroup(p);
            if (koPlayer == player && koPoint == p)
                reqv1.add_is_simple_ko(true);
            else
                reqv1.add_is_simple_ko(false);
            bool is_empty = group == groupEnd();
            if (is_empty)
            {
                reqv1.add_our_group_lib1(false);
                reqv1.add_our_group_lib2(false);
                reqv1.add_our_group_lib3_plus(false);
                reqv1.add_oppo_group_lib1(false);
                reqv1.add_oppo_group_lib2(false);
                reqv1.add_oppo_group_lib3_plus(false);
            } else
            {
                bool is_our = group->getPlayer() == player;
                std::size_t lib = group->getLiberty();
                if (is_our)
                {
                    reqv1.add_our_group_lib1(lib == 1);
                    reqv1.add_our_group_lib2(lib == 2);
                    reqv1.add_our_group_lib3_plus(lib >= 3);
                    reqv1.add_oppo_group_lib1(false);
                    reqv1.add_oppo_group_lib2(false);
                    reqv1.add_oppo_group_lib3_plus(false);
                } else
                {
                    reqv1.add_oppo_group_lib1(lib == 1);
                    reqv1.add_oppo_group_lib2(lib == 2);
                    reqv1.add_oppo_group_lib3_plus(lib >= 3);
                    reqv1.add_our_group_lib1(false);
                    reqv1.add_our_group_lib2(false);
                    reqv1.add_our_group_lib3_plus(false);
                }
            }
        });
        return reqv1;
    }

    template<std::size_t W, std::size_t H>
    auto Board<W, H>::generateRequestV2(Player player) -> gocnn::RequestV2
    {
        gocnn::RequestV2 reqv2;
        reqv2.set_board_size(W * H);
        reqv2.mutable_stone_color_our()->Reserve(reqv2.board_size());
        reqv2.mutable_stone_color_oppo()->Reserve(reqv2.board_size());
        reqv2.mutable_stone_color_empty()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_one()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_two()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_three()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_four()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_five()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_six()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_seven()->Reserve(reqv2.board_size());
        reqv2.mutable_turns_since_more()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_our_one()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_our_two()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_our_three()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_our_more()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_oppo_one()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_oppo_two()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_oppo_three()->Reserve(reqv2.board_size());
        reqv2.mutable_liberties_oppo_more()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_one()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_two()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_three()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_four()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_five()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_six()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_seven()->Reserve(reqv2.board_size());
        reqv2.mutable_capture_size_more()->Reserve(reqv2.board_size());
        reqv2.mutable_sensibleness()->Reserve(reqv2.board_size());
        reqv2.mutable_ko()->Reserve(reqv2.board_size());
        reqv2.mutable_border()->Reserve(reqv2.board_size());
        reqv2.mutable_position()->Reserve(reqv2.board_size());

        std::queue<PointType> placeHistory = getHistoryCopy();
        PointType lastOnePlace(-1, -1);
        PointType lastTwoPlace(-1, -1);
        PointType lastThreePlace(-1, -1);
        PointType lastFourPlace(-1, -1);
        PointType lastFivePlace(-1, -1);
        PointType lastSixPlace(-1, -1);
        PointType lastSevenPlace(-1, -1);
        switch (placeHistory.size())
        {
            case 0:
                break;
            case 1:
                lastOnePlace = placeHistory.front();
                placeHistory.pop();
                break;
            case 2:
                lastTwoPlace = placeHistory.front();
                placeHistory.pop();
                lastOnePlace = placeHistory.front();
                placeHistory.pop();
                break;
            case 3:
                lastThreePlace = placeHistory.front();
                placeHistory.pop();
                lastTwoPlace = placeHistory.front();
                placeHistory.pop();
                lastOnePlace = placeHistory.front();
                placeHistory.pop();
                break;
            case 4:
                lastFourPlace = placeHistory.front();
                placeHistory.pop();
                lastThreePlace = placeHistory.front();
                placeHistory.pop();
                lastTwoPlace = placeHistory.front();
                placeHistory.pop();
                lastOnePlace = placeHistory.front();
                placeHistory.pop();
                break;
            case 5:
                lastFivePlace = placeHistory.front();
                placeHistory.pop();
                lastFourPlace = placeHistory.front();
                placeHistory.pop();
                lastThreePlace = placeHistory.front();
                placeHistory.pop();
                lastTwoPlace = placeHistory.front();
                placeHistory.pop();
                lastOnePlace = placeHistory.front();
                placeHistory.pop();
                break;
            case 6:
                lastSixPlace = placeHistory.front();
                placeHistory.pop();
                lastFivePlace = placeHistory.front();
                placeHistory.pop();
                lastFourPlace = placeHistory.front();
                placeHistory.pop();
                lastThreePlace = placeHistory.front();
                placeHistory.pop();
                lastTwoPlace = placeHistory.front();
                placeHistory.pop();
                lastOnePlace = placeHistory.front();
                placeHistory.pop();
                break;
            case 7:
                lastSevenPlace = placeHistory.front();
                placeHistory.pop();
                lastSixPlace = placeHistory.front();
                placeHistory.pop();
                lastFivePlace = placeHistory.front();
                placeHistory.pop();
                lastFourPlace = placeHistory.front();
                placeHistory.pop();
                lastThreePlace = placeHistory.front();
                placeHistory.pop();
                lastTwoPlace = placeHistory.front();
                placeHistory.pop();
                lastOnePlace = placeHistory.front();
                placeHistory.pop();
                break;
            default:
                break;
        }
        PointType::for_all([&](PointType p) {
            bool isEmpty = (getPointState(p) == PointState::NA);
            bool isOurs = (getPointState(p) == getPointStateFromPlayer(player));
            auto group = *getPointGroup(p);
            std::size_t liberty = 0;
            std::size_t stoneCount = 0;
            if (!isEmpty)
            {
                liberty = group.getLiberty();
                stoneCount = group.getStoneCnt();
            }

            if (isEmpty)
            {
                reqv2.add_stone_color_our(false);
                reqv2.add_stone_color_oppo(false);
                reqv2.add_stone_color_empty(true);

                reqv2.add_liberties_our_one(false);
                reqv2.add_liberties_our_two(false);
                reqv2.add_liberties_our_three(false);
                reqv2.add_liberties_our_more(false);

                reqv2.add_liberties_oppo_one(false);
                reqv2.add_liberties_oppo_two(false);
                reqv2.add_liberties_oppo_three(false);
                reqv2.add_liberties_oppo_more(false);

                reqv2.add_self_atari_one(false);
                reqv2.add_self_atari_two(false);
                reqv2.add_self_atari_three(false);
                reqv2.add_self_atari_four(false);
                reqv2.add_self_atari_five(false);
                reqv2.add_self_atari_six(false);
                reqv2.add_self_atari_seven(false);
                reqv2.add_self_atari_more(false);

                reqv2.add_capture_size_one(false);
                reqv2.add_capture_size_two(false);
                reqv2.add_capture_size_three(false);
                reqv2.add_capture_size_four(false);
                reqv2.add_capture_size_five(false);
                reqv2.add_capture_size_six(false);
                reqv2.add_capture_size_seven(false);
                reqv2.add_capture_size_more(false);
            } else if (isOurs)
            {
                reqv2.add_stone_color_our(true);
                reqv2.add_stone_color_oppo(false);
                reqv2.add_stone_color_empty(false);

                reqv2.add_liberties_our_one(liberty == 1);
                reqv2.add_liberties_our_two(liberty == 2);
                reqv2.add_liberties_our_three(liberty == 3);
                reqv2.add_liberties_our_more(liberty >= 4);

                reqv2.add_liberties_oppo_one(false);
                reqv2.add_liberties_oppo_two(false);
                reqv2.add_liberties_oppo_three(false);
                reqv2.add_liberties_oppo_more(false);

                if (liberty == 1)
                {
                    reqv2.add_self_atari_one(stoneCount == 1);
                    reqv2.add_self_atari_two(stoneCount == 2);
                    reqv2.add_self_atari_three(stoneCount == 3);
                    reqv2.add_self_atari_four(stoneCount == 4);
                    reqv2.add_self_atari_five(stoneCount == 5);
                    reqv2.add_self_atari_six(stoneCount == 6);
                    reqv2.add_self_atari_seven(stoneCount == 7);
                    reqv2.add_self_atari_more(stoneCount >= 8);
                }
                else
                {
                    reqv2.add_self_atari_one(false);
                    reqv2.add_self_atari_two(false);
                    reqv2.add_self_atari_three(false);
                    reqv2.add_self_atari_four(false);
                    reqv2.add_self_atari_five(false);
                    reqv2.add_self_atari_six(false);
                    reqv2.add_self_atari_seven(false);
                    reqv2.add_self_atari_more(false);
                }

                reqv2.add_capture_size_one(false);
                reqv2.add_capture_size_two(false);
                reqv2.add_capture_size_three(false);
                reqv2.add_capture_size_four(false);
                reqv2.add_capture_size_five(false);
                reqv2.add_capture_size_six(false);
                reqv2.add_capture_size_seven(false);
                reqv2.add_capture_size_more(false);
            }
            else
            {
                reqv2.add_stone_color_our(false);
                reqv2.add_stone_color_oppo(true);
                reqv2.add_stone_color_empty(false);

                reqv2.add_liberties_our_one(false);
                reqv2.add_liberties_our_two(false);
                reqv2.add_liberties_our_three(false);
                reqv2.add_liberties_our_more(false);

                reqv2.add_liberties_oppo_one(liberty == 1);
                reqv2.add_liberties_oppo_two(liberty == 2);
                reqv2.add_liberties_oppo_three(liberty == 3);
                reqv2.add_liberties_oppo_more(liberty >= 4);

                reqv2.add_self_atari_one(false);
                reqv2.add_self_atari_two(false);
                reqv2.add_self_atari_three(false);
                reqv2.add_self_atari_four(false);
                reqv2.add_self_atari_five(false);
                reqv2.add_self_atari_six(false);
                reqv2.add_self_atari_seven(false);
                reqv2.add_self_atari_more(false);

                if (liberty == 1)
                {
                    reqv2.add_capture_size_one(stoneCount == 1);
                    reqv2.add_capture_size_two(stoneCount == 2);
                    reqv2.add_capture_size_three(stoneCount == 3);
                    reqv2.add_capture_size_four(stoneCount == 4);
                    reqv2.add_capture_size_five(stoneCount == 5);
                    reqv2.add_capture_size_six(stoneCount == 6);
                    reqv2.add_capture_size_seven(stoneCount == 7);
                    reqv2.add_capture_size_more(stoneCount >= 8);
                }
                else
                {
                    reqv2.add_capture_size_one(false);
                    reqv2.add_capture_size_two(false);
                    reqv2.add_capture_size_three(false);
                    reqv2.add_capture_size_four(false);
                    reqv2.add_capture_size_five(false);
                    reqv2.add_capture_size_six(false);
                    reqv2.add_capture_size_seven(false);
                    reqv2.add_capture_size_more(false);
                }
            }

            reqv2.add_turns_since_one(p == lastOnePlace);
            reqv2.add_turns_since_two(p == lastTwoPlace);
            reqv2.add_turns_since_three(p == lastThreePlace);
            reqv2.add_turns_since_four(p == lastFourPlace);
            reqv2.add_turns_since_five(p == lastFivePlace);
            reqv2.add_turns_since_six(p == lastSixPlace);
            reqv2.add_turns_since_seven(p == lastSevenPlace);
            if (!isEmpty && !(p == lastOnePlace ||
                    p == lastTwoPlace ||
                    p == lastThreePlace ||
                    p == lastFourPlace ||
                    p == lastFivePlace ||
                    p == lastSixPlace ||
                    p == lastSevenPlace))
                reqv2.add_turns_since_more(true);
            else
                reqv2.add_turns_since_more(false);

            reqv2.add_sensibleness(isTrueEye(p, player));

            reqv2.add_ko(koPlayer == player && koPoint == p);

            reqv2.add_border(p.is_left() || p.is_top() || p.is_right() || p.is_bottom());

            reqv2.add_position(exp(-0.5 * (pow((double)p.x - (double)(H - 1) / 2.0, 2) + pow((double)p.y - (double)(W - 1) / 2.0, 2))));
        });

        return reqv2;
    }


    template<std::size_t W, std::size_t H>
    auto Board<W, H>::generateRequestV2Bug(Player player) -> gocnn::RequestV2
    {
        gocnn::RequestV2 reqV2 = generateRequestV2(player);
        std::fill(reqV2.mutable_turns_since_one()->begin(), reqV2.mutable_turns_since_one()->end(), false);
        std::fill(reqV2.mutable_turns_since_two()->begin(), reqV2.mutable_turns_since_two()->end(), false);
        std::fill(reqV2.mutable_turns_since_three()->begin(), reqV2.mutable_turns_since_three()->end(), false);
        std::fill(reqV2.mutable_turns_since_four()->begin(), reqV2.mutable_turns_since_four()->end(), false);
        std::fill(reqV2.mutable_turns_since_five()->begin(), reqV2.mutable_turns_since_five()->end(), false);
        std::fill(reqV2.mutable_turns_since_six()->begin(), reqV2.mutable_turns_since_six()->end(), false);
        std::fill(reqV2.mutable_turns_since_seven()->begin(), reqV2.mutable_turns_since_seven()->end(), false);
        std::fill(reqV2.mutable_turns_since_more()->begin(), reqV2.mutable_turns_since_more()->end(), true);
        return reqV2;
    };
}

namespace std
{

    template<std::size_t W, std::size_t H>
    struct hash<board::Board<W, H>>
    {
    private:
        hash<board::BoardGrid<W, H>> h;
    public:
        std::size_t operator() (const board::Board<W, H> &b) const
        {
            return h(b.boardGrid_);
        }
    };
}
#endif //GO_AI_BOARD_CLASS_HPP
