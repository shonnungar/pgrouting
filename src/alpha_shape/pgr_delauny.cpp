/*PGR-GNU*****************************************************************
file: pgr_delauny.cpp

Copyright (c) 2018 pgRouting developers
Mail: project@pgrouting.org

Copyright (c) 2018 Celia Virginia Vergara Castillo
vicky_vergara@hotmail.com

------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

********************************************************************PGR-GNU*/

#include "alphaShape/pgr_delauny.hpp"

#include <vector>
#include <algorithm>

#include "cpp_common/bpoint.hpp"
#include "alphaShape/pgr_triangle.hpp"

namespace pgrouting {
namespace alphashape {

namespace detail {
void
cleanup_data(std::vector<Delauny_t> &delauny) {
        std::sort(delauny.begin(), delauny.end(),
                [](const Delauny_t &e1, const Delauny_t &e2)->bool {
                    return e1.pid < e2.pid;
                });
        std::stable_sort(delauny.begin(), delauny.end(),
                [](const Delauny_t &e1, const Delauny_t &e2)->bool {
                    return e1.tid < e2.tid;
                });
        delauny.erase(std::unique(delauny.begin(), delauny.end(),
                [](const Delauny_t &e1, const Delauny_t &e2)->bool {
                    return e1.tid == e2.tid && e1.pid == e2.pid;
                }), delauny.end());
}

size_t
point_idx(const std::vector<Bpoint> &points, Bpoint p1) {
    return std::find_if(points.begin(), points.end(),
            [&p1](const Bpoint &p)->bool {
                return boost::geometry::equals(p, p1);
            })  - points.begin();
}

}  // namespace detail

Pgr_delauny::Pgr_delauny(
             const std::vector<Bpoint> &p_points,
             const std::vector<Delauny_t> &p_delauny) :
    m_points(p_points),
    m_delauny(p_delauny) {
        /*
         * Inserting points from delauny inforamtion
         */

        for (auto &d : m_delauny) {
            Bpoint p(d.x, d.y);
            auto point_itr = std::find_if(m_points.begin(), m_points.end(),
                    [&p](const Bpoint &p1)->bool {
                        return boost::geometry::equals(p, p1);
                    });

            if (point_itr == m_points.end()) {
                /*
                 * Point is not found
                 */
                d.pid = m_points.size();
                m_points.push_back(p);
            } else {
                d.pid = point_itr - m_points.begin();
            }
        }

        /*
         * removing duplicate triangles information
         */
        detail::cleanup_data(m_delauny);

        /*
         * creating the triangles
         */
        for (size_t i = 0; i < m_delauny.size(); i = i + 3) {
            auto p1 = m_points[m_delauny[i].pid];
            auto p2 = m_points[m_delauny[i + 1].pid];
            auto p3 = m_points[m_delauny[i + 2].pid];
            auto tid = m_triangles.size();
            m_triangles.push_back(Pgr_triangle(p1, p2, p3));

            m_relation[detail::point_idx(m_points, p1)].push_back(tid);
            m_relation[detail::point_idx(m_points, p2)].push_back(tid);
            m_relation[detail::point_idx(m_points, p3)].push_back(tid);
        }
        log << *this;
}

void
Pgr_delauny::clear() {
    m_relation.clear();
    m_triangles.clear();
    m_points.clear();
}

std::ostream&
operator<<(std::ostream& os, const Pgr_delauny &d) {
    os << "Points\n";
    for (const auto p : d.m_points) {
        os << boost::geometry::wkt(p) << ", ";
    };

    os << "\nDelauny triangles\n";
    for (const auto t : d.m_triangles) {
        os << t << ",\n";
    }

    os << "\nrelation\n";
    for (const auto r : d.m_relation) {
        os << "\n" << r.first << "->" << boost::geometry::wkt(d.m_points[r.first]) << ": ";
        for (const auto e : r.second) {
            os << "\n   " << e << "->" << d.m_triangles[e];
        }
    }
    return os;
}

}  // namespace alphashape
}  // namespace pgrouting
