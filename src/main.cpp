#include <mpi.h>
#include <omp.h>
#include <cstdio>
#include <vector>
#include <queue>
#include <stack>
#include <functional>
#include <unordered_set>

#include <filesystem>
#include <chrono>

#include <pthread.h>
#include <unistd.h>
#include "../include/graph.h"
#include "../include/solution.h"
#include "../include/maxclique.h"

constexpr int INITIAL_NODE = 1;
constexpr int SOLUTION_FROM_WORKER = 2;
constexpr int RETURN = 3;
constexpr int RETURN_TIME_LIMIT = 4;

constexpr unsigned int NEW_UB = 6;
constexpr unsigned int NEW_LB = 9;

constexpr int value_idx = 0;
constexpr int type_idx = 1;

void* listen_for_ub_updates_from_root(void* arg);
void* listen_for_ub_updates_from_workers(void* arg);

void* compute_lb(void* graph_ptr);
void* timer_thread(void* arg);

void send_solution(const solution &sol, const int dest, const int tag, MPI_Comm comm) {
  // Create a buffer to hold all data
  std::vector<unsigned int> buffer(solution::dim + 2);

  // Pack the data: [color vector | tot_colors | next]
  std::copy(sol.color.begin(), sol.color.end(), buffer.begin());
  buffer[solution::dim] = sol.tot_colors;
  buffer[solution::dim + 1] = sol.next;

  // Send everything in a single call
  MPI_Send(buffer.data(), buffer.size(), MPI_UNSIGNED, dest, tag, comm);
}
void receive_solution(solution &sol, const int source, const int tag, MPI_Comm comm, MPI_Status &status) {
  // Create a buffer to receive all data
  std::vector<unsigned int> buffer(solution::dim + 2);

  // Receive everything in a single call
  MPI_Recv(buffer.data(), buffer.size(), MPI_UNSIGNED, source, tag, comm, &status);

  // Unpack the data
  sol.color.assign(buffer.begin(), buffer.begin() + solution::dim);
  sol.tot_colors = buffer[solution::dim];
  sol.next = buffer[solution::dim + 1];
}

void broadcast_graph(graph &g, const int root, MPI_Comm comm) {
  int rank;
  MPI_Comm_rank(comm, &rank);

  // Step 1: Broadcast the graph dimension from root
  unsigned int size;
  if (rank == root) size = static_cast<unsigned int>(graph::dim);

  MPI_Bcast(&size, 1, MPI_UNSIGNED, root, comm);

  if (rank != root) graph::dim = static_cast<size_t>(size);

  // Step 2: Resize adjacency matrix after receiving dimension
  if (rank != root) {
    g.m.resize(graph::dim, std::vector<bool>(graph::dim));
  }

  // Step 3: Prepare a flat buffer to store adjacency matrix
  std::vector<char> buffer(graph::dim * graph::dim);

  if (rank == root) {
    // Flatten the adjacency matrix for broadcasting
    for (size_t i = 0; i < graph::dim; ++i) {
      for (size_t j = 0; j < graph::dim; ++j) {
        buffer[i * graph::dim + j] = g.m[i][j] ? 1 : 0;  // Convert bool to char
      }
    }
  }

  // Step 4: Broadcast the adjacency matrix to all processes
  MPI_Bcast(buffer.data(), buffer.size(), MPI_CHAR, root, comm);

  // Step 5: Convert received buffer back into adjacency matrix
  if (rank != root) {
    for (size_t i = 0; i < graph::dim; ++i) {
      for (size_t j = 0; j < graph::dim; ++j) {
        g.m[i][j] = (buffer[i * graph::dim + j] != 0);
      }
    }
  }
}

int rank;
int size;

std::string instance_name;
unsigned int max_time;

auto start = std::chrono::high_resolution_clock::now();

int main(int argc, char** argv){

  graph g{};

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

  if (argc != 3) {
    if (rank == 0) std::cerr << "Usage:\nmpirun -n <number_of_processes> ./graph-coloring <path_to_input_graph_file> <seconds_time_limit>\n";
    MPI_Finalize();
    return 0;
  }

  try {
    max_time = std::stoul(argv[2]);
    if (rank == 0) std::cout << "Time limit: " << max_time << " (s)" << std::endl;
  } catch (const std::exception& e) {
    if (rank == 0) std::cerr << "Error: Invalid unsigned integer." << std::endl;
    MPI_Finalize();
    return 0;
  }

  if (size < 2) {
    if (rank == 0) std::cerr << "At least 2 processes (-n 2) are required!\n";
    MPI_Finalize();
    return 0;
  }


  std::string file_path = argv[1];

  // ----- initialize queue ----- //

  // rank 0 process initializes the fist queue, exploring solution space with BFS
  if (rank == 0) {

    // summon timer thread
    pthread_t timer;
    pthread_create(&timer, nullptr, timer_thread, &max_time);


    // initialize the graph and send it to other processes
    try {
      g = graph(file_path);

      std::filesystem::path path_obj(file_path);
      instance_name = path_obj.filename().string();
    } catch (std::exception &e) {
      std::cerr << e.what() << "\n";
      MPI_Abort(MPI_COMM_WORLD, 1);
      return 0;
    }

    // send the graph to other processes
    broadcast_graph(g, 0, MPI_COMM_WORLD);

    // give a reference of the graph to all instances of solution objects
    solution::attach_graph(&g);

    // start to look for a lower bound
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // Set the thread as detached
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Create a detached pthread
    pthread_t thread;
    pthread_create(&thread, &attr, compute_lb, &g);

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

    // at this point either the solution is found OR we can start assigning work to each process
    if( initial_q.empty() ) {
      std::cout << "==== Optimal Solution ====\n" << best_so_far << "==========================\n";
      std::cout << "NO PARALLELISM USED \nTERMINATING ALL PROCESSES\n" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, 69);
    }

    std::cout << "\nProcess " << rank << " generated an initial queue with " << initial_q.size() << " nodes.\n\n";
    std::cout << "Current color upper bound is: " << solution::colors_ub << "\n\n";
    std::cout << (size - 1) - initial_q.size() << " worker processes will do nothing.\n\n";

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

    std::cout << "Process 0 sent starting node to workers.\n\n";

    // start thread to listen to solutions found by worker threads
    pthread_t listener_thread;
    pthread_create(&listener_thread, nullptr, listen_for_ub_updates_from_workers, nullptr);

    // wait for workers to finish
    MPI_Barrier(MPI_COMM_WORLD);


    pthread_join(listener_thread, nullptr);

  }

  if(rank != 0) {

    // receive graph from root node
    broadcast_graph(g, 0, MPI_COMM_WORLD);

    solution::attach_graph(&g);


    solution sol_init_loc;

    // wait for initial node from proces 0
    MPI_Status status;
    receive_solution(sol_init_loc, 0, MPI_ANY_TAG, MPI_COMM_WORLD, status);

    bool keep_looping = true;

    // summon listener thread
    pthread_t listener_thread;
    pthread_create(&listener_thread, nullptr, listen_for_ub_updates_from_root, &keep_looping);

    // if a message was received with a tag different from zero, the worker thread does nothing
    if(status.MPI_TAG != INITIAL_NODE) {
      std::cout << "Process " << rank << " did not receive a node!\n\n";

    } else {
      //std::cout << "Process " << rank << " received the solution:\n" << sol_init_loc << std::endl;

      unsigned long int tot_solutions_generated = 0;
      std::stack<solution> q{};
      q.push(sol_init_loc);

      solution best_so_far;

      while(!q.empty() && keep_looping) {

        // pop the first element in the stack
        auto curr = q.top(); q.pop();
        tot_solutions_generated++;

        // early termination
        if(solution::colors_lb == solution::colors_ub) {
          printf("Process %d terminating early!\n\n", rank);
          break;
        }

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

      // for good measure
      if (best_so_far.is_final())
        send_solution(best_so_far, 0, SOLUTION_FROM_WORKER, MPI_COMM_WORLD);

      //std::cout << "Process " << rank << " ended computation!" << std::endl;

      // if the queue is not empty the tree was not fully explored, and we can no longer claim optimality
      // unless we exited the loop due to lb being equal to ub
      solution dummy_sol;
      if (!q.empty() && solution::colors_lb != solution::colors_ub) {
        send_solution(dummy_sol, 0, RETURN_TIME_LIMIT, MPI_COMM_WORLD);
        std::cout << "Process " << rank << " time is up!\n\n";
      } else if (q.empty()) {
        std::cout << "Process " << rank << " emptied queue!\n\n";
      }

    }

    // communicate to root process this process is done
    solution dummy;
    send_solution(dummy, 0, RETURN, MPI_COMM_WORLD);

    // let rank 0 node know computation is completed
    MPI_Barrier(MPI_COMM_WORLD);

    // wait return of listener thread
    pthread_join(listener_thread, nullptr);
  }


  //std::cout << "Process " << rank << " completed!" << std::endl;

  MPI_Finalize();

  return 0;
}

// this function is executed by a thread on each worker process
void* listen_for_ub_updates_from_root(void* arg) {
  bool* keep_looping = static_cast<bool*>(arg);

  while (true) {
    // Blocking call: Will wait here until rank 0 broadcasts a message
    unsigned int message[2];
    MPI_Bcast(message, 2, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    if (message[type_idx] == RETURN) break;

    if (message[type_idx] == RETURN_TIME_LIMIT) {
      *keep_looping = false;
      break;
    }

    if (message[type_idx] == NEW_LB) {
      const unsigned int new_lb = message[value_idx];
      solution::colors_lb = new_lb;

      //std::cout << "New color lb is " << new_lb << std::endl;
    }

    if (message[type_idx] == NEW_UB && message[value_idx] < solution::colors_ub) {
      solution::colors_ub = message[value_idx];
      //std::cout << "Process " << rank << " received new ub: " << solution::colors_ub << std::endl;
    }
  }

  return nullptr;

}

void* listen_for_ub_updates_from_workers(void* arg) {
  int worker_done = 0;
  solution best;

  bool optimality = true;

  // while at least one worker has not finished
  while (worker_done < size - 1) {
    MPI_Status status;
    solution new_best;
    receive_solution(new_best, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);

    if(status.MPI_TAG == RETURN) {
      worker_done += 1;
    }

    if(status.MPI_TAG == RETURN_TIME_LIMIT) {
      worker_done += 1;
      optimality = false;
    }

    if(status.MPI_TAG == SOLUTION_FROM_WORKER) {
      // if the new best solution is actually better
      if(new_best.tot_colors < solution::colors_ub) {
        // update upper bound in rank 0 memory
        solution::colors_ub = new_best.tot_colors;
        best = new_best;

        std::cout << "Process " << status.MPI_SOURCE << " sent solution:\n" << new_best << "\n";

        // broadcast new upperbound to workers
        unsigned int ub[2] = {solution::colors_ub, NEW_UB};
        MPI_Bcast(ub , 2, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
      }
    }
  }

  // broadcast workers we are done
  unsigned int done[2] = {0, RETURN};
  MPI_Bcast(done, 2, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;

  if (best.is_final() && optimality) {
    std::cout << "===== OPTIMAL SOLUTION =====\n" << best << "============================" << std::endl;
    best.write_to_file(instance_name, duration.count(), size, max_time, true);
  }
  else if (best.is_final() && !optimality) {
    std::cout << "===== SUB-OPT SOLUTION =====\n" << best << "============================" << std::endl;
    best.write_to_file(instance_name, duration.count(), size, max_time, false);
  }
  else
    std::cout << "No solutions found." << std::endl;

  std::cout << std::flush;

  return nullptr;

}

void* timer_thread(void* arg) {
  const unsigned int time_seconds = *static_cast<unsigned int*>(arg);
  sleep(time_seconds);

  // broadcast workers we are done
  unsigned int done[2] = {0, RETURN_TIME_LIMIT};
  MPI_Bcast(done, 2, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

  return nullptr;
}


// Compute lower bound via max clique
void* compute_lb(void* graph_ptr) {
  graph* g = static_cast<graph*>(graph_ptr);
  int max_clique_size = find_max_clique(*g);

  printf("===== LOWER BOUND (Max Clique): %d =====\n\n", max_clique_size);

  // Update the lower bound of whole system (solution::colors_lb)
  solution::colors_lb = max_clique_size;

  // Communicate new lb to all processes via MPI
  unsigned int message[2] = { static_cast<unsigned int>(max_clique_size), NEW_LB };
  MPI_Bcast(message, 2, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

  return nullptr;
}

