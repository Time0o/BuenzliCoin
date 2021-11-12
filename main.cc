#include "node.h"

int main()
{
  bm::Node node { "0.0.0.0", 5050 };
  node.run();
}
