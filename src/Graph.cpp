#include "Graph.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <cstdlib>
#include <ctime> 

GraphWrapper::GraphWrapper(int n) : g(n) {}

void GraphWrapper::addEdge(int u, int v) {
    boost::add_edge(u, v, g);
}

GraphWrapper GraphWrapper::generateRandomGraph(int n, double p) {
    GraphWrapper gw(n);
    std::srand(std::time(nullptr));
    // Iterate over each potential edge (only once for each undirected edge)
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if ((double)std::rand() / RAND_MAX < p) {
                boost::add_edge(u, v, gw.g);
            }
        }
    }
    return gw;
}

const GraphWrapper::Graph& GraphWrapper::getGraph() const {
    return g;
}

GraphWrapper::Graph& GraphWrapper::getGraph() {
    return g;
}
