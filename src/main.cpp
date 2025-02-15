#include <cstdio>
#include <vector>

#include <queue>
#include <stack>

#include "../include/graph.h"
#include "../include/solution.h"

constexpr unsigned int N = 4;

int main(){

  unsigned int tot_solutions_generated = 0;

  graph<N> g("../inputs/g.col");
  std::cout << g << std::endl;

  solution<N>::g = &g;

  const solution<N> s{};
  std::stack<solution<N>> q{};
  q.push(s);

  while(!q.empty()) {
    auto curr = q.top(); q.pop();
    tot_solutions_generated++;

    std::cout << curr << std::endl;

    if(!curr.is_final()) {
      auto tmp = curr.get_next();
      // add children to the STACK in reverse order, to ensure the first one of the list is popped next
      for(auto child = tmp.rbegin(); child != tmp.rend(); ++child)
        q.push(*child);
    } else {
      solution<N>::colors_ub = std::min(solution<N>::colors_ub, curr.tot_colors);
    }
  }

  std::cout << "Tot solutions generated:\t" << tot_solutions_generated << std::endl;

  std::cout << "Upperbound on colors:\t\t" << solution<N>::colors_ub << std::endl;
  return 0;
}