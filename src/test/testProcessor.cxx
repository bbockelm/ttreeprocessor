
#include <atomic>
#include <iostream>

#include "TTreeProcessor.h"


class MyMapper final : public ROOT::TTreeProcessorMapper<std::tuple<int>, float, int, double> {
public:
  MyMapper(int starter_count) : count(starter_count) {}

  virtual std::tuple<int> map(float, int, double) const override __attribute__((always_inline)) {count++; return 1;}

  virtual void finalize() override {std::cout << "There were " << count << "events.\n";}

private:
  // Hmm... this suppresses move-constructors.
  //mutable std::atomic<int> count;
  mutable int count;
};

int main(int argc, char *argv[]) {

  ROOT::TTreeProcessor<std::tuple<float, int, double>> processor({"a", "b", "c"});
  processor
  .map([](float x, int y, double z) -> std::tuple<int, float> {std::cout << "First mapper got X input of " << x << "\n"; return std::make_tuple(y, x);})
  .map([](int x, float y) -> std::tuple<int> {std::cout << "Second mapper got X input of " << x << "\n"; return std::make_tuple(x*x+1);})
  .process();

  ROOT::TTreeProcessor<std::tuple<float, int, double>, MyMapper> processor2({"a", "b", "c"}, MyMapper(argc));
  processor2.process();

  return 0;
}
