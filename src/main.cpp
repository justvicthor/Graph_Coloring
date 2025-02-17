#include <mpi.h>

#include <cstdio>
#include <vector>
#include <queue>
#include <stack>

#include "../include/graph.h"
#include "../include/solution.h"

constexpr unsigned int N = 5;

void master_job();

int main(int argc, char** argv){

  // ----- graph initialization ----- //

  // read graph from file
  graph<N> g("../inputs/g.col");
  //graph<N> g(0.8);

  //std::cout << g << std::endl;

  // give a reference of the graph to all instances of solution objects
  solution<N>::g = &g;

  // ----- graph initialization ----- //

  // ----- init MPI ----- //

  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided < MPI_THREAD_MULTIPLE) {
    printf("MPI does not support multiple threads properly!\n");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // get tot number of processes and rank for each process
  int size, rank; MPI_Comm_size(MPI_COMM_WORLD, &size); MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // create MPI data_type for solution class
  MPI_Datatype mpi_solution;
  create_MPI_Type_solution<N>(&mpi_solution);

  // ----- init MPI ----- //

  // ----- initialize queue ----- //

  // rank 0 process initializes the fist queue, exploring solution space with BFS
  if (rank == 0) {
    std::queue<solution<N>> initial_q{};

    const solution<N> s{};
    initial_q.push(s);

    solution<N> best_so_far;

    while(!initial_q.empty()) {

      // pop the first element in the stack
      auto curr = initial_q.front(); initial_q.pop();

      if(!curr.is_final()) {

        // prune internal nodes that require more (or as many) colors than the current known upperbound
        if(curr.tot_colors >= solution<N>::colors_ub) continue;

        // generate children nodes
        auto tmp = curr.get_next();

        // add them to the queue, unless it gets longer than the number of processes available
        if(initial_q.size() + tmp.size() <= size - 1) {
          for(auto & child : tmp)
            initial_q.push(child);
        } else {
          initial_q.push(curr);
          break;
        }

      } else if (curr.tot_colors < solution<N>::colors_ub) {
        // if the current solution is better than the previous one (or if it is the first optimal solution)

        // update the upper bound, the current best solution and print it
        solution<N>::colors_ub = curr.tot_colors;
        best_so_far = curr;
        std::cout << curr << std::endl;
      }
    }

    // at this point either the queue is found OR we can start assigning work to each process
    if( initial_q.empty() ) {
      std::cout << "==== Optimal Solution ====\n" << best_so_far << "==========================\n";
      std::cout << "NO PARALLELISM USED \nTERMINATING ALL PROCESSES\n" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, 69);
    }

    std::cout << "Process " << rank << " generated an initial queue with " << initial_q.size() << " nodes." << std::endl;
    std::cout << "Current color upper bound is: " << solution<N>::colors_ub << std::endl;
    std::cout << (size - 1) - initial_q.size() << " worker processes will do nothing." << std::endl;

    // Print the queue
    /*
    auto copy = initial_q;
    while (!copy.empty()) {
      std::cout << copy.front() << " ";  // Stampa l'elemento in testa
      copy.pop();  // Rimuove l'elemento
    }
    */

    // main process now dispatches each node to a worker process
    int i = 1;
    while ( !initial_q.empty() ) {
      MPI_Send(&initial_q.front(), 1, mpi_solution, i, 0, MPI_COMM_WORLD);
      initial_q.pop();
      i++;
    }
  }

  if(rank != 0) {
    solution<N> sol;
    MPI_Recv(&sol, 1, mpi_solution, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    std::cout << "Process " << rank << " received the solution:\n" << sol << std::endl;
  }


  std::cout << "Process " << rank << " completed!" << std::endl;


  MPI_Type_free(&mpi_solution);
  MPI_Finalize();
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