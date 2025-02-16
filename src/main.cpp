#include <iostream>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include "CliqueColoring.hpp"
#include "Graph.hpp"

using namespace graphalgo;

int main() {
    int numThreads = omp_get_max_threads();
    std::cout << "Using up to " << numThreads << " threads for parallel sections.\n";

    // Ask the user for graph parameters.
    int vertices;
    double edgeProbability;
    std::cout << "Enter number of vertices: ";
    std::cin >> vertices;
    std::cout << "Enter edge probability (0 to 1): ";
    std::cin >> edgeProbability;

    // Create a random graph using GraphWrapper.
    GraphWrapper gw = GraphWrapper::generateRandomGraph(vertices, edgeProbability);
    auto& g = gw.getGraph();
    std::cout << "Graph: Vertices = " << boost::num_vertices(g)
              << ", Edges = " << boost::num_edges(g) << "\n";

    // Compute average degree.
    double avgDegree = computeAverageDegree(g);
    std::cout << "Average Degree: " << avgDegree << "\n";

    // Precompute neighbor bitsets (needed for both sequential and parallel algorithms).
    auto neighborBitsets = computeNeighborBitsets(g);

    // For adaptive strategy, we choose a threshold (you can adjust as needed).
    const double degreeThreshold = 50.0;
    std::vector<GraphWrapper::Vertex> clique;
    double cliqueTime = 0.0;
    std::string methodUsed;

    // We also run both sequential (bitset-optimized) and parallel versions on the dense graph
    // to compute speedup and efficiency. We will do this only if the graph is dense.
    double seqTime = 0.0, parTime = 0.0;
    std::vector<GraphWrapper::Vertex> cliqueSeq, cliquePar;

    if (avgDegree < degreeThreshold) {
        std::cout << "Adaptive algorithm: Using Sequential Bitset Optimized method.\n";
        auto [result, time] = measureExecutionTime(sequentialGreedyCliqueExpansionBitset, g, neighborBitsets);
        clique = result;
        cliqueTime = time;
        methodUsed = "Sequential Bitset Optimized";
    } else {
        std::cout << "Adaptive algorithm: Using Improved Parallel method.\n";
        // For dense graphs, we run both methods to compare:
        auto [resultSeq, timeSeq] = measureExecutionTime(sequentialGreedyCliqueExpansionBitset, g, neighborBitsets);
        cliqueSeq = resultSeq;
        seqTime = timeSeq;

        auto [resultPar, timePar] = measureExecutionTime(improvedParallelGreedyCliqueExpansionAll, g, neighborBitsets);
        cliquePar = resultPar;
        parTime = timePar;

        // We'll choose the one with the lower time as our adaptive output.
        if (seqTime <= parTime) {
            clique = cliqueSeq;
            cliqueTime = seqTime;
            methodUsed = "Sequential Bitset Optimized";
        } else {
            clique = cliquePar;
            cliqueTime = parTime;
            methodUsed = "Improved Parallel";
        }
    }
    
    std::cout << "Adaptive algorithm (" << methodUsed << "): Clique expansion found a clique of size: " 
              << clique.size() << " in " << cliqueTime << " seconds.\n";

    // Run greedy coloring.
    auto [colors, timeColoring] = measureExecutionTime(greedyColoring, g);
    int numColors = *std::max_element(colors.begin(), colors.end()) + 1;
    std::cout << "Greedy coloring used " << numColors << " colors in " 
              << timeColoring << " seconds.\n";

    // --- Calculate Speedup and Efficiency (if dense) ---
    // We'll only compute speedup and efficiency if we have both sequential and parallel times.
    double speedup = 0.0, efficiency = 0.0;
    if (avgDegree >= degreeThreshold && seqTime > 0.0 && parTime > 0.0) {
        speedup = seqTime / parTime;
        efficiency = (speedup / numThreads) * 100.0;
        std::cout << "Speedup (Sequential/Parallel): " << speedup 
                  << ", Efficiency: " << efficiency << "%\n";
    } else {
        std::cout << "Speedup/Efficiency not computed for sparse graph.\n";
    }

    // --- Export Results to a Text File ---
    std::ofstream outFile("results.txt");
    if (outFile.is_open()) {
        outFile << std::fixed << std::setprecision(6);
        outFile << "Graph Coloring Benchmark Results\n";
        outFile << "==================================\n\n";
        outFile << "Graph Parameters:\n";
        outFile << "  Vertices: " << vertices << "\n";
        outFile << "  Edge Probability: " << edgeProbability << "\n";
        outFile << "  Edges: " << boost::num_edges(g) << "\n";
        outFile << "  Average Degree: " << avgDegree << "\n\n";
        
        outFile << "Adaptive Clique Expansion:\n";
        outFile << "  Method Used: " << methodUsed << "\n";
        outFile << "  Clique Size: " << clique.size() << "\n";
        outFile << "  Time: " << cliqueTime << " seconds\n\n";

        outFile << "Greedy Coloring:\n";
        outFile << "  Colors Used: " << numColors << "\n";
        outFile << "  Time: " << timeColoring << " seconds\n\n";

        if (avgDegree >= degreeThreshold && seqTime > 0.0 && parTime > 0.0) {
            outFile << "Performance Metrics (Dense Graph):\n";
            outFile << "  Sequential Time: " << seqTime << " seconds\n";
            outFile << "  Parallel Time: " << parTime << " seconds\n";
            outFile << "  Speedup: " << speedup << "\n";
            outFile << "  Efficiency: " << efficiency << "%\n";
        } else {
            outFile << "Performance Metrics not computed for sparse graph.\n";
        }
        
        outFile.close();
        std::cout << "Results exported to results.txt in the build folder.\n";
    } else {
        std::cerr << "Error: Unable to open results.txt for writing.\n";
    }
    
    return 0;
}
