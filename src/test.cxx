
#include "TTreeProcessor.h"

int main(int argc, char *argv[]) {

  ROOT::TTreeProcessor<std::tuple<float, int, double>> processor({"a", "b", "c"});
  processor.map([](float x, int y, double z) -> std::tuple<float, int> {return std::make_tuple(x, y);})
  .process();

  return 0;
}
