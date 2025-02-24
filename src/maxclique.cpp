#include "../include/maxclique.h"
#include <vector>
#include <omp.h>

// Defines the depth to which OpenMP tasks are created
#define TASK_DEPTH_THRESHOLD 2

// Recursive Bron-Kerbosch function with pivoting and pruning
// R: current clique, P: candidates, X: already considered
// 'max_clique' is updated in critical section
void bronKerbosch(const std::vector<int>& R,
                  const std::vector<int>& P,
                  const std::vector<int>& X,
                  const graph &g,
                  int &max_clique,
                  int depth) {
    // If P and X are empty, R is a max clique
    if (P.empty() && X.empty()) {
        #pragma omp critical
        {
            if (R.size() > static_cast<size_t>(max_clique))
                max_clique = R.size();
        }
        return;
    }

    // Pruning
    if (R.size() + P.size() <= static_cast<size_t>(max_clique))
        return;

    // Pivot selection: we choose a vertex from P U X
    int u = -1;
    if (!P.empty() || !X.empty())
        u = (!P.empty() ? P[0] : X[0]);

    // Compute P \ N(u)
    std::vector<int> P_without_neighbors;
    for (int v : P) {
        if (!g(u, v))
            P_without_neighbors.push_back(v);
    }

    // Iterate on selected candidates
    for (int v : P_without_neighbors) {
        std::vector<int> newR = R;
        newR.push_back(v);

        // newP = P ∩ N(v)
        std::vector<int> newP;
        for (int w : P) {
            if (g(v, w))
                newP.push_back(w);
        }

        // newX = X ∩ N(v)
        std::vector<int> newX;
        for (int w : X) {
            if (g(v, w))
                newX.push_back(w);
        }

        // Parallelize recursive calls
        if (depth < TASK_DEPTH_THRESHOLD) {
            #pragma omp task firstprivate(newR, newP, newX, depth)
            {
                bronKerbosch(newR, newP, newX, g, max_clique, depth + 1);
            }
        } else {
            bronKerbosch(newR, newP, newX, g, max_clique, depth + 1);
        }
    }
    #pragma omp taskwait
}

int find_max_clique(const graph &g) {
    int max_clique = 0;
    std::vector<int> R, P, X;
    // Inizialize P with all vertices [0, dim-1]
    for (int i = 0; i < static_cast<int>(graph::dim); i++) {
        P.push_back(i);
    }
    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            bronKerbosch(R, P, X, g, max_clique, 0);
        }
    }
    return max_clique;
}
