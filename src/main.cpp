#include <cstdio>

#include "../include/graph.h"

int main(){

  graph<5> g("../inputs/g.col");
  std::cout << g << std::endl;

  return 0;
}