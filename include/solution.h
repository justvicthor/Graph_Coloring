#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>
#include <mpi.h>
#include <unordered_set>

#include "graph.h"

struct solution {
    // nodes are numbered   [0 to dim-1]
    // colors are numbered  [1 to dim (at most)]

    static size_t dim;

    static graph * g;

    // upper bound on number of colors to use
    static unsigned int colors_ub;
    static unsigned int colors_lb;

    // this array contains the color (repr as an integer) of each node: 0 -> color not assigned yet
    std::vector<unsigned int> color;

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
    [[nodiscard]] std::vector<solution> get_next() const;

    friend std::ostream& operator<<(std::ostream& os, const solution& sol);

    static void attach_graph(graph * g);

private:

    // constructor for the "child" of the solution
    solution(const solution& parent, unsigned int node_to_color, unsigned int node_color);

    [[nodiscard]] unsigned int node_with_most_colored_neighbors() const;
};

#endif //SOLUTION_H
