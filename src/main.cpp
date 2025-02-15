#include <cstdio>
#include <vector>
#include <queue>

#include "../include/graph.h"
#include "../include/solution.h"

constexpr unsigned int N = 4;

int main(){

  unsigned int tot_solutions_generated = 0;

  graph<N> g("../inputs/g.col");
  std::cout << g << std::endl;

  const solution<N> s{};
  std::queue<solution<N>> q{};
  q.push(s);

  while(!q.empty()) {
    auto curr = q.front(); q.pop();
    tot_solutions_generated++;

    std::cout << curr << std::endl;

    if(!curr.is_final()) {
      auto tmp = curr.get_next();
      for(auto& child : tmp)
        q.push(child);
    }
  }

  std::cout << "Tot solutions generated:\t" << tot_solutions_generated << std::endl;

  std::cout << "Upperbound on colors:\t\t" << solution<N>::colors_ub << std::endl;
  return 0;
}