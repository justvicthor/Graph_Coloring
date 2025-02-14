#ifndef GRAPH_H
#define GRAPH_H

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

template<unsigned int dim>
struct graph {

    explicit graph(const std::string& file_path);

    // incident matrix
    std::array<std::array<bool, dim>, dim> m;

    bool operator()(unsigned int i, unsigned int j) const;

    friend std::ostream& operator<<(std::ostream& os, const graph<dim>& g) {
        for (unsigned int i = 0; i < dim; ++i) {
            for (unsigned int j = 0; j < dim; ++j) {
                os << g.m[i][j] << "  ";
            }
            os << "\n";
        }
        return os;
    }
};

#include "../src/graph.tpp"

#endif //GRAPH_H
