//
// Created by lz on 11/2/16.
//

#ifndef GO_AI_GRID_POINT_HPP
#define GO_AI_GRID_POINT_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <compressed_grid.hpp>

namespace board
{
    template<std::size_t W, std::size_t H>
    struct GridPoint: public compgrid::GridPoint<W, H>
    {
    private:
        using BaseT = compgrid::GridPoint<W, H>;
    public:
        GridPoint() = default;
        GridPoint(char x, char y): BaseT(x, y) {}
        template<typename FT>
        static void for_all(FT f)
        {
            GridPoint p(0, 0);
            for (; !p.is_bottom() || !p.is_right(); ++p)
                f(p);
            f(p);
        }
        GridPoint& left()
        {
            static_cast<BaseT&>(*this).left();
            return *this;
        }
        GridPoint &right()
        {
            static_cast<BaseT&>(*this).right();
            return *this;
        }
        GridPoint &up()
        {
            static_cast<BaseT&>(*this).up();
            return *this;
        }
        GridPoint &down()
        {
            static_cast<BaseT&>(*this).down();
            return *this;
        }
        GridPoint left_point() const
        {
            GridPoint gp = *this;
            return gp.left();
        }
        GridPoint right_point() const
        {
            GridPoint gp = *this;
            return gp.right();
        }
        GridPoint up_point() const
        {
            GridPoint gp = *this;
            return gp.up();
        }
        GridPoint down_point() const
        {
            GridPoint gp = *this;
            return gp.down();
        }
        GridPoint left_up_point() const
        {
            GridPoint gp = *this;
            return gp.left().up();
        }
        GridPoint left_down_point() const
        {
            GridPoint gp = *this;
            return gp.left().down();
        }
        GridPoint right_up_point() const
        {
            GridPoint gp = *this;
            return gp.right().up();
        }
        GridPoint right_down_point() const
        {
            GridPoint gp = *this;
            return gp.right().down();
        }
        GridPoint next_point() const
        {
            GridPoint gp = *this;
            return ++gp;
        }
        GridPoint prev_point() const
        {
            GridPoint gp = *this;
            return --gp;
        }

        bool adjacent_to(GridPoint gp)
        {
            bool adj = false;
            for_each_adjacent([gp, &adj](GridPoint adjP) {
                adj |= adjP == gp;
            });
            return adj;
        }

        template<typename FT>
        void for_each_adjacent(FT f) const
        {
            if (!this->is_left()) f(left_point());
            if (!this->is_top()) f(up_point());
            if (!this->is_right()) f(right_point());
            if (!this->is_bottom()) f(down_point());
        }
        std::vector<GridPoint> get_adjacent_point() const
        {
            std::vector<GridPoint> v;
            if (!this->is_left()) v.push_back(left_point());
            if (!this->is_top()) v.push_back(up_point());
            if (!this->is_right()) v.push_back(right_point());
            if (!this->is_bottom()) v.push_back(down_point());
            return v;
        }
        template<typename FT>
        void for_each_diag(FT f) const
        {
            if (!this->is_left() && !this->is_top()) f(left_up_point());
            if (!this->is_left() && !this->is_bottom()) f(left_down_point());
            if (!this->is_right() && !this->is_top()) f(right_up_point());
            if (!this->is_right() && !this->is_bottom()) f(right_down_point());
        }
        std::vector<GridPoint> get_diag_point() const
        {
            std::vector<GridPoint> v;
            if (!this->is_left() && !this->is_top()) v.push_back(left_up_point());
            if (!this->is_left() && !this->is_bottom()) v.push_back(left_down_point());
            if (!this->is_right() && !this->is_top()) v.push_back(right_up_point());
            if (!this->is_right() && !this->is_bottom()) v.push_back(right_down_point());
            return v;
        }
        template<typename FT>
        void for_each_wrap8(FT f) const
        {
            if (!this->is_left()) f(left_point());
            if (!this->is_top()) f(up_point());
            if (!this->is_right()) f(right_point());
            if (!this->is_bottom()) f(down_point());
            if (!this->is_left() && !this->is_top()) f(left_up_point());
            if (!this->is_left() && !this->is_bottom()) f(left_down_point());
            if (!this->is_right() && !this->is_top()) f(right_up_point());
            if (!this->is_right() && !this->is_bottom()) f(right_down_point());
        }
        std::vector<GridPoint> get_wrap8_point() const
        {
            std::vector<GridPoint> v;
            if (!this->is_left()) v.push_back(left_point());
            if (!this->is_top()) v.push_back(up_point());
            if (!this->is_right()) v.push_back(right_point());
            if (!this->is_bottom()) v.push_back(down_point());
            if (!this->is_left() && !this->is_top()) v.push_back(left_up_point());
            if (!this->is_left() && !this->is_bottom()) v.push_back(left_down_point());
            if (!this->is_right() && !this->is_top()) v.push_back(right_up_point());
            if (!this->is_right() && !this->is_bottom()) v.push_back(right_down_point());
            return v;
        }
        bool operator ==(const GridPoint &other) const
        {
            return this->x == other.x && this->y == other.y;
        }
        bool operator !=(const GridPoint &other) const
        {
            return !(*this == other);
        }
    };
}
#endif //GO_AI_GRID_POINT_HPP
