#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <cstdlib>
#include <ctime>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

class GraphWrapper {
public:
    // Underlying Boost graph type.
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    
    // Constructor to create a graph with n vertices.
    GraphWrapper(int n);
    
    // Add an edge between vertices u and v.
    void addEdge(int u, int v);
    
    // Static function to generate a random graph with n vertices and edge probability p.
    static GraphWrapper generateRandomGraph(int n, double p);
    
    // Getters for the underlying graph.
    const Graph& getGraph() const;
    Graph& getGraph();
    
private:
    Graph g;
};

#endif // GRAPH_HPP
