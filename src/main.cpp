#include <cstdio>
#include <vector>

#include <queue>
#include <stack>

#include "../include/graph.h"
#include "../include/solution.h"

constexpr unsigned int N = 30;

int main(){

  unsigned long int tot_solutions_generated = 0;

  //graph<N> g("../inputs/g.col");
  graph<N> g(0.8);
  std::cout << g << std::endl;

  solution<N>::g = &g;

  const solution<N> s{};
  std::stack<solution<N>> q{};
  q.push(s);

  solution<N> best_so_far;

  while(!q.empty()) {
    auto curr = q.top(); q.pop();
    tot_solutions_generated++;

    if(!curr.is_final() && curr.tot_colors < solution<N>::colors_ub) {
      auto tmp = curr.get_next();
      // add children to the STACK in reverse order, to ensure the first one of the list is popped next
      for(auto child = tmp.rbegin(); child != tmp.rend(); ++child)
        q.push(*child);

    } else {
      // check if the current solution is better than the previous one
      if (curr.tot_colors < solution<N>::colors_ub) {
        solution<N>::colors_ub = curr.tot_colors;
        best_so_far = curr;
        std::cout << curr << std::endl;
      }
    }

    //std::cout << curr << std::endl;
  }

  std::cout << "==== Optimal Solution ====\n" << best_so_far << "==========================\n";
  std::cout << "Tot solutions explored:\t" << tot_solutions_generated << std::endl;

  return 0;
}