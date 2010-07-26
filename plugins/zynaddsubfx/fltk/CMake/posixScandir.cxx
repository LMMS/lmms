
#include <dirent.h>

int func (const char *d, dirent ***list, void *sort) {
  int n = scandir(d, list, 0, (int(*)(const dirent **, const dirent **))sort);
}

int main() {
  return 0;
}
