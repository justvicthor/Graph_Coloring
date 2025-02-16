#include <cstdio>
#include <vector>

#include <queue>
#include <stack>

#include "../include/graph.h"
#include "../include/solution.h"

constexpr unsigned int N = 4;

void graph_preprocessing();
void master_job();
void slave_job();

int main(){

  // read graph from file
  graph<N> g("../inputs/g.col");
  //graph<N> g(0.8);
  std::cout << g << std::endl;

  // give a reference of the graph to all instances of solution objects
  solution<N>::g = &g;

  master_job();

  return 0;
}

// this function looks for solution that require STRICTLY LESS THAN solution<dim>::colors_ub colors
void master_job() {
  unsigned long int tot_solutions_generated = 0;

  const solution<N> s{};
  std::stack<solution<N>> q{};
  q.push(s);

  solution<N> best_so_far;

  while(!q.empty()) {

    // pop the first element in the stack
    auto curr = q.top(); q.pop();
    tot_solutions_generated++;

    if(!curr.is_final()) {

      // prune internal nodes that require more (or as many) colors than the current known upperbound
      if(curr.tot_colors >= solution<N>::colors_ub) continue;

      // generate children nodes
      auto tmp = curr.get_next();
      // add them to the STACK in reverse order, to ensure the first one of the list is popped next
      for(auto child = tmp.rbegin(); child != tmp.rend(); ++child)
        q.push(*child);

    } else if (curr.tot_colors < solution<N>::colors_ub) {
      // if the current solution is better than the previous one (or if it is the first optimal solution)

      // update the upper bound, the current best solution and print it
      solution<N>::colors_ub = curr.tot_colors;
      best_so_far = curr;
      std::cout << curr << std::endl;
    }
  }

  if(best_so_far.is_final()) {
    std::cout << "==== Optimal Solution ====\n" << best_so_far << "==========================\n";
    std::cout << "Tot solutions explored:\t" << tot_solutions_generated << std::endl << std::endl;
  } else {
    std::cout << "No solutions found with less than " << solution<N>::colors_ub << " colors." << std::endl;
  }

}