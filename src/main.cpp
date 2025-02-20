#include <mpi.h>

#include <cstdio>
#include <vector>
#include <queue>
#include <stack>

#include <pthread.h>
#include <unistd.h>

#include "../include/graph.h"
#include "../include/solution.h"

constexpr unsigned int N = 49;

constexpr int INITIAL_NODE = 1;
constexpr int SOLUTION_FROM_WORKER = 2;
constexpr int RETURN = 3;

void master_job();
void* listen_for_ub_updates_from_root(void* arg);
void* listen_for_ub_updates_from_workers(void* arg);

void send_solution(const solution &sol, int dest, int tag, MPI_Comm comm) {
  // Create a buffer to hold all data
  std::vector<unsigned int> buffer(solution::dim + 2);

  // Pack the data: [color vector | tot_colors | next]
  std::copy(sol.color.begin(), sol.color.end(), buffer.begin());
  buffer[solution::dim] = sol.tot_colors;
  buffer[solution::dim + 1] = sol.next;

  // Send everything in a single call
  MPI_Send(buffer.data(), buffer.size(), MPI_UNSIGNED, dest, tag, comm);
}

void receive_solution(solution &sol, int source, int tag, MPI_Comm comm, MPI_Status &status) {
  // Create a buffer to receive all data
  std::vector<unsigned int> buffer(solution::dim + 2);

  // Receive everything in a single call
  MPI_Recv(buffer.data(), buffer.size(), MPI_UNSIGNED, source, tag, comm, &status);

  // Unpack the data
  sol.color.assign(buffer.begin(), buffer.begin() + solution::dim);
  sol.tot_colors = buffer[solution::dim];
  sol.next = buffer[solution::dim + 1];
}

int rank;
int size;

// TODO : FIX WILE TRUE FOR PROCESS 0
// TODO : FIX NON-WORKING PROCESSES

int main(int argc, char** argv){

  // ----- graph initialization ----- //

  // read graph from file
  graph g("../inputs/anna.col");
  //graph<N> g(0.8);

  //std::cout << g << std::endl;

  // give a reference of the graph to all instances of solution objects
  solution::attach_graph(&g);

  // ----- graph initialization ----- //

  // ----- init MPI ----- //

  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided < MPI_THREAD_MULTIPLE) {
    printf("MPI does not support multiple threads properly!\n");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // get tot number of processes and rank for each process
  MPI_Comm_size(MPI_COMM_WORLD, &size); MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // ----- init MPI ----- //

  // ----- initialize queue ----- //

  // rank 0 process initializes the fist queue, exploring solution space with BFS
  if (rank == 0) {
    std::queue<solution> initial_q{};

    const solution s{};
    initial_q.push(s);

    solution best_so_far;

    while(!initial_q.empty()) {

      // pop the first element in the stack
      auto curr = initial_q.front(); initial_q.pop();

      if(!curr.is_final()) {

        // prune internal nodes that require more (or as many) colors than the current known upperbound
        if(curr.tot_colors >= solution::colors_ub) continue;

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

      } else if (curr.tot_colors < solution::colors_ub) {
        // if the current solution is better than the previous one (or if it is the first optimal solution)

        // update the upper bound, the current best solution and print it
        solution::colors_ub = curr.tot_colors;
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
    std::cout << "Current color upper bound is: " << solution::colors_ub << std::endl;
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
      send_solution(initial_q.front(), i, INITIAL_NODE, MPI_COMM_WORLD);
      initial_q.pop();
      i++;
    }

    const solution dummy_solution{};
    while ( i < size ) {
      send_solution(dummy_solution, i, 0, MPI_COMM_WORLD);
      i++;
    }

    std::cout << "Process 0 sent starting node to workers." << std::endl;

    // start thread to listen to solutions found by worker threads
    pthread_t listener_thread;
    pthread_create(&listener_thread, nullptr, listen_for_ub_updates_from_workers, nullptr);

    // wait for workers to finish
    MPI_Barrier(MPI_COMM_WORLD);


    pthread_join(listener_thread, nullptr);

  }

  if(rank != 0) {
    solution sol_init_loc;

    // wait for initial node from proces 0
    MPI_Status status;
    receive_solution(sol_init_loc, 0, MPI_ANY_TAG, MPI_COMM_WORLD, status);

    // summon listener thread
    pthread_t listener_thread;
    pthread_create(&listener_thread, nullptr, listen_for_ub_updates_from_root, nullptr);

    // if a message was received with a tag different from zero, the worker thread does nothing
    if(status.MPI_TAG != INITIAL_NODE) {
      std::cout << "Process " << rank << " did not receive a node!" << std::endl;

    } else {
      std::cout << "Process " << rank << " received the solution:\n" << sol_init_loc << std::endl;

      unsigned long int tot_solutions_generated = 0;
      std::stack<solution> q{};
      q.push(sol_init_loc);

      solution best_so_far;

      while(!q.empty()) {

        // pop the first element in the stack
        auto curr = q.top(); q.pop();
        tot_solutions_generated++;

        if(!curr.is_final()) {

          // prune internal nodes that require more (or as many) colors than the current known upperbound
          if(curr.tot_colors >= solution::colors_ub) continue;

          // generate children nodes
          auto tmp = curr.get_next();
          // add them to the STACK in reverse order, to ensure the first one of the list is popped next
          for(auto child = tmp.rbegin(); child != tmp.rend(); ++child)
            q.push(*child);

        } else if (curr.tot_colors < solution::colors_ub) {
          // if the current solution is better than the previous one (or if it is the first optimal solution)

          // update the upper bound, the current best solution and print it
          solution::colors_ub = curr.tot_colors;
          best_so_far = curr;

          // communicate new best solution root process (rank 0)
          send_solution(best_so_far, 0, SOLUTION_FROM_WORKER, MPI_COMM_WORLD);
        }
      }

      //if(best_so_far.is_final()) {
      //  std::cout << "==== Optimal Solution ====\n" << best_so_far << "==========================\n";
      //  std::cout << "Tot solutions explored:\t" << tot_solutions_generated << std::endl << std::endl;
      //} else {
      //  std::cout << "No solutions found with less than " << solution<N>::colors_ub << " colors." << std::endl;
      //}

      std::cout << "Process " << rank << " ended computation!" << std::endl;

    }

    // communicate to root process this process is done
    solution dummy;
    send_solution(dummy, 0, RETURN, MPI_COMM_WORLD);

    // let rank 0 node know computation is completed
    MPI_Barrier(MPI_COMM_WORLD);

    // wait return of listener thread
    pthread_join(listener_thread, nullptr);
  }


  std::cout << "Process " << rank << " completed!" << std::endl;

  MPI_Finalize();

  return 0;
}

// this function is executed by a thread on each worker process
void* listen_for_ub_updates_from_root(void* arg) {

  while (true) {
    // Blocking call: Will wait here until rank 0 broadcasts a message
    unsigned int new_ub;
    MPI_Bcast(&new_ub, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    if (new_ub == RETURN) break;

    if (new_ub < solution::colors_ub) {
      solution::colors_ub = new_ub;
      std::cout << "Process " << rank << " received new ub: " << solution::colors_ub << std::endl;
    }
  }

  return nullptr;

}

void* listen_for_ub_updates_from_workers(void* arg) {
  int worker_done = 0;
  solution best;

  // while at least one worker has not finished
  while (worker_done < size - 1) {
    MPI_Status status;
    solution new_best;
    receive_solution(new_best, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);

    if(status.MPI_TAG == RETURN) {
      worker_done += 1;
    }

    if(status.MPI_TAG == SOLUTION_FROM_WORKER) {
      // if the new best solution is actually better
      if(new_best.tot_colors < solution::colors_ub) {
        // update upper bound in rank 0 memory
        solution::colors_ub = new_best.tot_colors;
        best = new_best;

        // broadcast new upperbound to workers
        MPI_Bcast(&solution::colors_ub, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
      }

      std::cout << "Process " << status.MPI_SOURCE << " sent solution:\n" << new_best << "\n";
    }
  }

  // broadcast workers we are done
  MPI_Bcast((void *)(&RETURN), 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
  std::cout << "===== OPTIMAL SOLUTION =====\n" << best << "============================" << std::endl;

  return nullptr;

}