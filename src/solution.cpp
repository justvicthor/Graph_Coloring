#include "solution.h"

#include <cassert>

size_t solution::dim = -1;
unsigned int solution::colors_ub = -1;
unsigned int solution::colors_lb = 0;
graph * solution::g = nullptr;




solution::solution() : color(solution::dim), tot_colors(0), next(0) {
}


bool solution::is_final() const {
    if (next < solution::dim) return false;
    return true;
}


[[nodiscard]] std::vector<solution> solution::get_next() const {
    assert(this->is_final() == false && "Cannot generate children of a complete solution!");

    const unsigned int node_to_color = this->node_with_most_colored_neighbors();
    const unsigned int colors = tot_colors + 1;
    //const unsigned int colors = dim;

    std::vector<solution> children;
    children.reserve(colors);

    for (unsigned int i = 1; i <= colors; ++i) {
        // generate a child node and check if the color assignment is valid. If it is, add it to the list only if the
        // total number of colors used id no more than the current known upper bound.
        if (const solution child = solution(*this, node_to_color, i);
            child.is_valid(node_to_color) && child.tot_colors <= colors_ub) {
            children.emplace_back(child);
        }
    }

    return children;
}


solution::solution(const solution& parent, const unsigned int node_to_color, const unsigned int node_color) {
    // copy the color assignment from the parent solution
    this->color = parent.color;

    // copy parameters
    tot_colors = node_color > parent.tot_colors ? parent.tot_colors + 1 : parent.tot_colors;
    next = parent.next + 1;

    // color the node
    color[node_to_color] = node_color;
}


bool solution::is_valid(const unsigned int node_to_check) const {

    const unsigned int i = node_to_check;
    for (unsigned int j = 0; j < solution::dim; ++j)
        // if two nodes are adjacent and are colored the same the solution is not valid.
        if (g->operator()(i, j) == true && color[i] == color[j]) return false;

    return true;

}

void solution::attach_graph(graph * g) {
    solution::dim = graph::dim;
    solution::colors_ub = dim + 1;

    solution::g = g;
}

[[nodiscard]] unsigned int solution::node_with_most_colored_neighbors() const {
    size_t max_colored_neighbors = 0;
    unsigned int best_node = 0;

    for (size_t i = 0; i < dim; ++i) {
        if (color[i] != 0) continue; // Skip already colored nodes

        std::unordered_set<unsigned int> neighbor_colors;
        for (size_t j = 0; j < dim; ++j) {
            if ((*g)(i, j) && color[j] != 0) {
                neighbor_colors.insert(color[j]);
            }
        }

        if (neighbor_colors.size() > max_colored_neighbors) {
            max_colored_neighbors = neighbor_colors.size();
            best_node = i;
        }
    }

    return best_node;
}
