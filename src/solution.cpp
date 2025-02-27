#include "solution.h"
#include <omp.h>

#include <cassert>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>



size_t solution::dim = -1;
unsigned int solution::colors_ub = -1;
unsigned int solution::colors_lb = 0;
graph * solution::g = nullptr;




solution::solution() : color(solution::dim), tot_colors(0), next(0) {
}


bool solution::is_final() const {
    if (next < solution::dim) return false;
    return true;
}


[[nodiscard]] std::vector<solution> solution::get_next() const {
    assert(this->is_final() == false && "Cannot generate children of a complete solution!");

    const unsigned int node_to_color = this->node_with_most_colored_neighbors();
    const unsigned int colors = tot_colors + 1;
    //const unsigned int colors = dim;

    std::vector<solution> children;
    children.reserve(colors);

    for (unsigned int i = 1; i <= colors; ++i) {
        // generate a child node and check if the color assignment is valid. If it is, add it to the list only if the
        // total number of colors used id no more than the current known upper bound.
        if (const solution child = solution(*this, node_to_color, i);
            child.is_valid(node_to_color) && child.tot_colors <= colors_ub) {
            children.emplace_back(child);
        }
    }

    return children;
}


solution::solution(const solution& parent, const unsigned int node_to_color, const unsigned int node_color) {
    // copy the color assignment from the parent solution
    this->color = parent.color;

    // copy parameters
    tot_colors = node_color > parent.tot_colors ? parent.tot_colors + 1 : parent.tot_colors;
    next = parent.next + 1;

    // color the node
    color[node_to_color] = node_color;
}


bool solution::is_valid(const unsigned int node_to_check) const {

    const unsigned int i = node_to_check;
    for (unsigned int j = 0; j < solution::dim; ++j)
        // if two nodes are adjacent and are colored the same the solution is not valid.
        if (g->operator()(i, j) == true && color[i] == color[j]) return false;

    return true;

}

void solution::attach_graph(graph * g) {
    solution::dim = graph::dim;
    solution::colors_ub = dim + 1;

    solution::g = g;
}

[[nodiscard]] unsigned int solution::node_with_most_colored_neighbors() const {
    size_t max_colored_neighbors = 0;
    unsigned int best_node = 0;

    // Parallel reduction to find the node with the most colored neighbors
    #pragma omp parallel
    {
        size_t local_max_colored_neighbors = 0;
        unsigned int local_best_node = 0;

        #pragma omp for nowait
        for (size_t i = 0; i < dim; ++i) {
            if (color[i] != 0) continue; // Skip already colored nodes

            std::unordered_set<unsigned int> neighbor_colors;
            for (size_t j = 0; j < dim; ++j) {
                if ((*g)(i, j) && color[j] != 0) {
                    neighbor_colors.insert(color[j]);
                }
            }

            size_t neighbor_count = neighbor_colors.size();
            if (neighbor_count > local_max_colored_neighbors) {
                local_max_colored_neighbors = neighbor_count;
                local_best_node = i;
            }
        }

        // Reduce to find the global best node
        #pragma omp critical
        {
            if (local_max_colored_neighbors > max_colored_neighbors) {
                max_colored_neighbors = local_max_colored_neighbors;
                best_node = local_best_node;
            }
        }
    }

    return best_node;
}

std::ostream& operator<<(std::ostream& os, const solution& sol) {
    os << "Solution:\t\t[ ";

    if (solution::dim > 5) {
        os << "... ]\n";
    } else {
        for (unsigned int i = 0; i < solution::dim - 1; ++i)
            os << sol.color[i] << ", ";
        os << sol.color[solution::dim - 1] << " ]\n";
    }

    os << "Next:\t\t\t" << sol.next << "\n";
    os << "Total colors:\t\t" << sol.tot_colors << "\n";
    os << "Color ub:\t\t" << solution::colors_ub << "\n";
    os << "Color lb:\t\t" << solution::colors_lb << "\n";

    return os;
}

/*void solution::write_to_file(const std::string& instance_name, unsigned int time_taken, int n_processes, int time_limit_seconds, bool is_optimal) const {
    std::ofstream outfile;

    if (is_optimal)
        outfile.open("/out_opt/" + instance_name + ".output");
    else
        outfile.open("/out/" + instance_name + ".output");


    if (!outfile) {
        std::cerr << "Error opening output file!" << std::endl;
        return;
    }

    outfile << "problem_instance_file_name: " << instance_name << "\n";
    outfile << "cmd_line: mpirun -n " << n_processes << " ./graph-coloring ../inputs/" << instance_name << " " << time_limit_seconds <<"\n";
    outfile << "solver_version: v1.0.1\n";
    outfile << "number_of_vertices: " << dim << "\n";
    outfile << "number_of_edges: " << graph::edges << "\n";
    outfile << "time_limit_sec: " << time_limit_seconds << "\n";
    outfile << "number_of_worker_processes: " << n_processes << "\n";
    outfile << "number_of_cores_per_worker: 2\n";
    outfile << "wall_time_sec: " << time_taken << "\n";
    outfile << "is_within_time_limit: " << (time_taken < time_limit_seconds ? "true" : "false") << "\n";
    outfile << "number_of_colors: " << tot_colors << "\n";

    for (size_t i = 0; i < color.size(); ++i) {
        outfile << i << " " << color[i] << "\n";
    }

    outfile.close();
}*/

void solution::write_to_file(const std::string& instance_name, unsigned int time_taken, int n_processes, int time_limit_seconds, bool is_optimal) const {
    std::ofstream outfile;

    std::string base_dir = std::filesystem::current_path();  // Current directory
    std::string folder = base_dir + "/" + (is_optimal ? "out_opt" : "out");
    std::string filepath = folder + "/" + instance_name + ".output";

    // Check folder's existence
    struct stat info;
    if (stat(folder.c_str(), &info) != 0) {
        if (mkdir(folder.c_str(), 0777) != 0) {
            std::cerr << "Error: Could not create directory " << folder << std::endl;
            return;
        }
    }

    // Open the file
    outfile.open(filepath);
    if (!outfile) {
        std::cerr << "Error opening output file: " << filepath << std::endl;
        return;
    }

    outfile << "problem_instance_file_name: " << instance_name << "\n";
    outfile << "cmd_line: srun -n " << n_processes << " ./graph-coloring ../inputs/" << instance_name << " " << time_limit_seconds <<"\n";
    outfile << "solver_version: v1.0.1\n";
    outfile << "number_of_vertices: " << dim << "\n";
    outfile << "number_of_edges: " << graph::edges << "\n";
    outfile << "time_limit_sec: " << time_limit_seconds << "\n";
    outfile << "number_of_worker_processes: " << n_processes << "\n";
    outfile << "number_of_cores_per_worker: 2\n";
    outfile << "wall_time_sec: " << time_taken << "\n";
    outfile << "is_within_time_limit: " << (time_taken < time_limit_seconds ? "true" : "false") << "\n";
    outfile << "number_of_colors: " << tot_colors << "\n";

    for (size_t i = 0; i < color.size(); ++i) {
        outfile << i << " " << color[i] << "\n";
    }

    outfile.close();
}

