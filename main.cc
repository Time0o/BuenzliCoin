#include "node.h"

int main()
{
  bm::Node node { "http://localhost:5050" };

  node.run();
  node.block();
}
