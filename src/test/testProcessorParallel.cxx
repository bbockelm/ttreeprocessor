
#include <atomic>
#include <iostream>

#include "TTreeProcessor.h"


int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr <<"Usage: " << argv[0] << " fname [fname...] \n";
    return 1;
  }

  std::vector<TFile*> tfiles; tfiles.reserve(argc-1);
  for (int idx=1; idx<argc; idx++) {
    tfiles.push_back(TFile::Open(argv[idx]));
  }

  ROOT::TTreeProcessor<std::tuple<float, int, double>> processor({"a", "b", "c"});
  processor
  .map([](float x, int y, double z) -> std::tuple<int, float> {return std::make_tuple(y, x);})
  .filter([](int x, float y) {return y <= 5;})
  .map([](int x, float y) -> std::tuple<int> {std::cout << "Apply map to " << y << "\n"; return std::make_tuple(x*x+1);})
  .count()
  .processParallel("T", tfiles);

  return 0;
}
