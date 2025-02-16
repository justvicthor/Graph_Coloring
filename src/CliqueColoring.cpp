#include "CliqueColoring.hpp"
#include <algorithm>
#include <omp.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace graphalgo {


    double computeAverageDegree(const GraphWrapper::Graph& g) {
        double totalDegree = 0.0;
        int n = boost::num_vertices(g);
        for (auto it = boost::vertices(g).first; it != boost::vertices(g).second; ++it) {
            totalDegree += boost::degree(*it, g);
        }
        return (n > 0) ? totalDegree / n : 0.0;
    }
    
    std::vector<std::bitset<MAX_VERTICES>> computeNeighborBitsets(const GraphWrapper::Graph& g) {
        int n = boost::num_vertices(g);
        std::vector<std::bitset<MAX_VERTICES>> bitsets(n);
        for (int v = 0; v < n; v++) {
            for (auto vp = boost::adjacent_vertices(v, g); vp.first != vp.second; ++vp.first) {
                bitsets[v].set(*vp.first, true);
            }
        }
        return bitsets;
    }
    
    std::vector<GraphWrapper::Vertex> improvedParallelGreedyCliqueExpansionAll(
        const GraphWrapper::Graph& g, 
        const std::vector<std::bitset<MAX_VERTICES>>& neighborBitsets)
    {
        std::vector<GraphWrapper::Vertex> globalBestClique;
        auto vpair = boost::vertices(g);
        std::vector<GraphWrapper::Vertex> vertexList(vpair.first, vpair.second);
        
        // Precompute degrees for each vertex.
        std::vector<size_t> degrees;
        degrees.reserve(vertexList.size());
        for (const auto& v : vertexList) {
            degrees.push_back(boost::degree(v, g));
        }
        
        // Sort vertices by descending degree.
        std::sort(vertexList.begin(), vertexList.end(), [&](GraphWrapper::Vertex a, GraphWrapper::Vertex b) {
            return degrees[a] > degrees[b];
        });
        
        // Parallel region: each thread expands a clique using bitset intersection.
        #pragma omp parallel
        {
            std::vector<GraphWrapper::Vertex> threadBestClique;
            #pragma omp for schedule(dynamic)
            for (size_t i = 0; i < vertexList.size(); ++i) {
                GraphWrapper::Vertex v = vertexList[i];
                std::vector<GraphWrapper::Vertex> currentClique { v };
                std::bitset<MAX_VERTICES> cliqueCandidates = neighborBitsets[v];
                
                // Collect neighbors of v and sort them by descending degree.
                std::vector<GraphWrapper::Vertex> neighbors;
                for (auto vp = boost::adjacent_vertices(v, g); vp.first != vp.second; ++vp.first) {
                    neighbors.push_back(*vp.first);
                }
                std::sort(neighbors.begin(), neighbors.end(), [&](GraphWrapper::Vertex a, GraphWrapper::Vertex b) {
                    return degrees[a] > degrees[b];
                });
                
                // For each candidate (in sorted order), if it is still in the candidate set, add it and update the candidate set.
                for (GraphWrapper::Vertex u : neighbors) {
                    if (cliqueCandidates.test(u)) {
                        currentClique.push_back(u);
                        cliqueCandidates &= neighborBitsets[u];
                    }
                }
                if (currentClique.size() > threadBestClique.size()) {
                    threadBestClique = std::move(currentClique);
                }
            }
            #pragma omp critical
            {
                if (threadBestClique.size() > globalBestClique.size()) {
                    globalBestClique = std::move(threadBestClique);
                }
            }
        }
        return globalBestClique;
    }
    
// Standard Sequential Greedy Clique Expansion (non-bitset-optimized).
std::vector<GraphWrapper::Vertex> sequentialGreedyCliqueExpansion(const GraphWrapper::Graph& g) {
    std::vector<GraphWrapper::Vertex> bestClique;
    auto vpair = boost::vertices(g);
    std::vector<GraphWrapper::Vertex> vertexList(vpair.first, vpair.second);
    
    // Sort vertices by descending degree.
    std::sort(vertexList.begin(), vertexList.end(), [&](GraphWrapper::Vertex a, GraphWrapper::Vertex b) {
        return boost::degree(a, g) > boost::degree(b, g);
    });
    
    for (size_t i = 0; i < vertexList.size(); ++i) {
        GraphWrapper::Vertex v = vertexList[i];
        std::vector<GraphWrapper::Vertex> currentClique { v };
        
        std::vector<GraphWrapper::Vertex> neighbors;
        for (auto vp = boost::adjacent_vertices(v, g); vp.first != vp.second; ++vp.first)
            neighbors.push_back(*vp.first);
        std::sort(neighbors.begin(), neighbors.end(), [&](GraphWrapper::Vertex a, GraphWrapper::Vertex b) {
            return boost::degree(a, g) > boost::degree(b, g);
        });
        
        for (GraphWrapper::Vertex u : neighbors) {
            bool isAdjacentToAll = true;
            for (GraphWrapper::Vertex w : currentClique) {
                if (!boost::edge(u, w, g).second) {
                    isAdjacentToAll = false;
                    break;
                }
            }
            if (isAdjacentToAll)
                currentClique.push_back(u);
        }
        if (currentClique.size() > bestClique.size())
            bestClique = currentClique;
    }
    return bestClique;
}

// Bitset-Optimized Sequential Greedy Clique Expansion.
std::vector<GraphWrapper::Vertex> sequentialGreedyCliqueExpansionBitset(
    const GraphWrapper::Graph& g, const std::vector<std::bitset<MAX_VERTICES>>& neighborBitsets)
{
    std::vector<GraphWrapper::Vertex> bestClique;
    auto vpair = boost::vertices(g);
    std::vector<GraphWrapper::Vertex> vertexList(vpair.first, vpair.second);
    
    // Sort vertices by descending degree.
    std::sort(vertexList.begin(), vertexList.end(), [&](GraphWrapper::Vertex a, GraphWrapper::Vertex b) {
        return boost::degree(a, g) > boost::degree(b, g);
    });
    
    for (size_t i = 0; i < vertexList.size(); ++i) {
        GraphWrapper::Vertex v = vertexList[i];
        std::vector<GraphWrapper::Vertex> currentClique { v };
        
        std::vector<GraphWrapper::Vertex> neighbors;
        for (auto vp = boost::adjacent_vertices(v, g); vp.first != vp.second; ++vp.first)
            neighbors.push_back(*vp.first);
        std::sort(neighbors.begin(), neighbors.end(), [&](GraphWrapper::Vertex a, GraphWrapper::Vertex b) {
            return boost::degree(a, g) > boost::degree(b, g);
        });
        
        // Use precomputed bitsets for fast membership testing.
        for (GraphWrapper::Vertex u : neighbors) {
            bool isAdjacentToAll = true;
            for (GraphWrapper::Vertex w : currentClique) {
                if (!neighborBitsets[w].test(u)) {
                    isAdjacentToAll = false;
                    break;
                }
            }
            if (isAdjacentToAll)
                currentClique.push_back(u);
        }
        if (currentClique.size() > bestClique.size())
            bestClique = currentClique;
    }
    return bestClique;
}


// Improved Greedy Coloring Using Fixed-Size Bitset.
std::vector<int> greedyColoring(const GraphWrapper::Graph& g) {
    size_t n = boost::num_vertices(g);
    std::vector<int> colors(n, -1);
    
    // Build a list of vertices.
    std::vector<GraphWrapper::Vertex> vertexList;
    auto vpair = boost::vertices(g);
    for (auto it = vpair.first; it != vpair.second; ++it)
        vertexList.push_back(*it);
    
    // Sort vertices by descending degree.
    std::sort(vertexList.begin(), vertexList.end(), [&](GraphWrapper::Vertex a, GraphWrapper::Vertex b) {
        return boost::degree(a, g) > boost::degree(b, g);
    });
    
    // For each vertex, assign the smallest available color.
    for (GraphWrapper::Vertex v : vertexList) {
        std::bitset<MAX_FIXED_COLORS> usedColors;
        usedColors.reset();
        auto adjPair = boost::adjacent_vertices(v, g);
        for (auto it = adjPair.first; it != adjPair.second; ++it) {
            GraphWrapper::Vertex u = *it;
            if (colors[u] != -1)
                usedColors.set(colors[u], true);
        }
        int color = 0;
        while (color < MAX_FIXED_COLORS && usedColors.test(color))
            ++color;
        colors[v] = color;
    }
    return colors;
}

} // namespace graphalgo
