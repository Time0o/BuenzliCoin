#include "hasher.h"

int main()
{
  hash::SHA256Hasher hasher;
  hasher.hash("hello world");
}
