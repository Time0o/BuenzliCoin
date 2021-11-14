#include "node.h"

using namespace bm;

int main()
{
  Node node {
    "0.0.0.0", 5050,
    "0.0.0.0", 5051
  };

  node.run();
}
