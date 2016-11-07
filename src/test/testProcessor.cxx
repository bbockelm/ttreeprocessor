
#include <atomic>
#include <iostream>

#include "TTreeProcessor.h"


class MyMapper final : public ROOT::TTreeProcessorMapper<std::tuple<int>, float, int, double> {
public:
  MyMapper(int starter_count) : count(starter_count) {}

  std::tuple<int> map(float, int, double) const noexcept __attribute__((always_inline)) {count++; return 1;}

  bool finalize() {std::cout << "There were " << count << "events.\n"; return true;}

private:
  // Hmm... this suppresses move-constructors.
  //mutable std::atomic<int> count;
  mutable int count;
};

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr <<"Usage: " << argv[0] << " fname\n";
    return 1;
  }

  ROOT::TTreeProcessor<std::tuple<float, int, double>> processor_filter({"a", "b", "c"});
  processor_filter
  .filter([](float x, int y, double z) -> bool {if (x>5) {std::cout << "Filtering out input of " << x << "\n";} return x <= 5;})
  .process("T", {TFile::Open(argv[1])});

  ROOT::TTreeProcessor<std::tuple<float, int, double>> processor({"a", "b", "c"});
  processor
  .map([](float x, int y, double z) -> std::tuple<int, float> {std::cout << "First mapper got X input of " << x << "\n"; return std::make_tuple(y, x);})
  .filter([](int x, float y) {if (y > 5) {std::cout << "Filtering out input of " << y << "\n";} return y <= 5;})
  .map([](int x, float y) -> std::tuple<int> {std::cout << "Second mapper got X input of " << x << "\n"; return std::make_tuple(x*x+1);})
  .map([](int x) -> std::tuple<int> {std::cout << "Third mapper got X input of " << x << "\n"; return std::make_tuple(x*x+1);})
  .process("T", {TFile::Open(argv[1])});

  ROOT::TTreeProcessor<std::tuple<float, int, double>, MyMapper> processor2({"a", "b", "c"}, MyMapper(argc));
  processor2.process("T", {TFile::Open(argv[1])});

  return 0;
}
