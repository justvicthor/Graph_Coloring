#ifndef GRAPH_H
#define GRAPH_H

#include <array>
#include <fstream>
#include <random>
#include <iostream>
#include <sstream>
#include <string>

struct graph {

    explicit graph(const std::string& file_path);

    explicit graph(size_t d, double density);

    static size_t dim;

    // incident matrix
    std::vector<std::vector<bool>> m;

    bool operator()(unsigned int i, unsigned int j) const;

    friend std::ostream& operator<<(std::ostream& os, const graph& g) {
        for (unsigned int i = 0; i < graph::dim; ++i) {
            for (unsigned int j = 0; j < graph::dim; ++j) {
                os << g.m[i][j] << "  ";
            }
            os << "\n";
        }
        return os;
    }
};

#endif //GRAPH_H
