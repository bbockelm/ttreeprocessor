
#include <atomic>
#include <iostream>

#include "TTreeProcessor.h"

using floatv = ROOT::floatv;
using maskv  = ROOT::maskv;

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr <<"Usage: " << argv[0] << " fname\n";
    return 1;
  }

  ROOT::TTreeProcessor<std::tuple<float>> processor(std::make_tuple("a"));
  processor
    .map([](maskv m, floatv in) -> std::tuple<floatv> {std::cout << "New vectorized tuple: " << in << "\n"; return {2*in};})
    .process("T", {TFile::Open(argv[1])});

  return 0;
}
