#ifndef CLIQUE_COLORING_TPP
#define CLIQUE_COLORING_TPP

#include <omp.h>
#include <utility>
#include <forward_list>
#include <utility>
#include <chrono>

namespace graphalgo {

template <typename F, typename... Args>
auto measureExecutionTime(F func, Args&&... args)
    -> std::pair<decltype(func(std::forward<Args>(args)...)), double>
{
    double start = omp_get_wtime();
    auto result = func(std::forward<Args>(args)...);
    double end = omp_get_wtime();
    return {result, end - start};
}

} // namespace graphalgo

#endif // CLIQUE_COLORING_TPP
