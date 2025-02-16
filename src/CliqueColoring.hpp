#ifndef CLIQUE_COLORING_HPP
#define CLIQUE_COLORING_HPP

#include <vector>
#include <bitset>
#include "Graph.hpp"

// Global constants for bitset sizes.
const int MAX_VERTICES = 10000;      // For neighbor bitsets.
const int MAX_FIXED_COLORS = 512;      // For greedy coloring.

namespace graphalgo {

// Compute the average degree of the graph.
double computeAverageDegree(const GraphWrapper::Graph& g);

// Precompute neighbor bitsets for each vertex.
std::vector<std::bitset<MAX_VERTICES>> computeNeighborBitsets(const GraphWrapper::Graph& g);

std::vector<GraphWrapper::Vertex> sequentialGreedyCliqueExpansionBitset(
    const GraphWrapper::Graph& g, const std::vector<std::bitset<MAX_VERTICES>>& neighborBitsets);

// Improved parallel greedy clique expansion using bitset intersection (all improvements integrated).
std::vector<GraphWrapper::Vertex> improvedParallelGreedyCliqueExpansionAll(
    const GraphWrapper::Graph& g, 
    const std::vector<std::bitset<MAX_VERTICES>>& neighborBitsets);

// Improved greedy coloring using a fixed-size bitset.
std::vector<int> greedyColoring(const GraphWrapper::Graph& g);

// Generic timing wrapper (template function).
template <typename F, typename... Args>
auto measureExecutionTime(F func, Args&&... args)
    -> std::pair<decltype(func(std::forward<Args>(args)...)), double>;

} // namespace graphalgo

#include "CliqueColoring.tpp" // Template implementation.

#endif // CLIQUE_COLORING_HPP
