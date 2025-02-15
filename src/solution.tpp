#pragma once

#include <cassert>

template<unsigned int dim>
unsigned int solution<dim>::colors_ub = dim;

template<unsigned int dim>
solution<dim>::solution() : color{}, tot_colors(0), next(0) {}

template<unsigned int dim>
bool solution<dim>::is_final() const {
    for (unsigned int i = 0; i < dim; ++i)
        if (color[i] == 0) return false;
    return true;
}

template<unsigned int dim>
std::vector<solution<dim>> solution<dim>::get_next() const {
    assert(this->is_final() == false && "Cannot generate children of a complete solution!");

    const unsigned int node_to_color = this->next;
    const unsigned int colors = tot_colors + 1;
    //const unsigned int colors = dim;

    std::vector<solution<dim>> children;
    children.reserve(colors);

    for (unsigned int i = 1; i <= colors; ++i)
        children.emplace_back(solution(*this, node_to_color, i));

    return children;
}

template<unsigned int dim>
solution<dim>::solution(const solution<dim>& parent, unsigned int node_to_color, unsigned int node_color) {
    // copy the color assignment from the parent solution
    std::copy(std::begin(parent.color), std::end(parent.color), std::begin(this->color));

    // copy parameters
    tot_colors = node_color > parent.tot_colors ? parent.tot_colors + 1 : parent.tot_colors;
    next = node_to_color + 1 >= dim ? -1 : node_to_color + 1;

    // color the node
    color[node_to_color] = node_color;
}

template<unsigned int dim>
bool solution<dim>::is_valid(unsigned int node_to_check) const {

}