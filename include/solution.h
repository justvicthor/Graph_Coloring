#ifndef SOLUTION_H
#define SOLUTION_H

#include <array>
#include <vector>

#include "graph.h"

template<unsigned int dim>
struct solution {
    // nodes are numbered   [0 to dim-1]
    // colors are numbered  [1 to dim (at most)]

    static graph<dim>* g;

    // upper bound on number of colors to use
    static unsigned int colors_ub;

    // this array contains the color (repr as an integer) of each node: 0 -> color not assigned yet
    std::array<unsigned int, dim> color;

    // total number of colors used
    unsigned int tot_colors;

    // index of the first node without a color
    unsigned int next;

    // constructor for an empty solution
    solution();

    // returns true if all nodes are assigned a color
    bool is_final() const;

    // checks whether a solution is valid, with respect with a single node
    // i.e. checks whether a node has neighbours of the same color
    bool is_valid(unsigned int node_to_check) const;

    // returns a list of solutions, "children" of this, each one has a different color for the selected node
    std::vector<solution<dim>> get_next() const;

    friend std::ostream& operator<<(std::ostream& os, const solution<dim>& sol) {
        os << "Solution:\t[ ";
        for (unsigned int i = 0; i < dim - 1; ++i)
            os << sol.color[i] << ", ";
        os << sol.color[dim - 1] << " ]\n";
        os << "Total colors:\t" << sol.tot_colors << "\n";
        os << "Next:\t\t" << sol.next << "\n";
        os << "Color ub:\t" << colors_ub << "\n";

        return os;
    }

private:

    // constructor for the "child" of the solution
    solution(const solution<dim>& parent, const unsigned int node_to_color, const unsigned int node_color);
};

#include "../src/solution.tpp"

#endif //SOLUTION_H
