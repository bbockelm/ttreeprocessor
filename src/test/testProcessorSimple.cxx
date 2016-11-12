
#include <atomic>
#include <iostream>

#include "TTreeProcessor.h"


class MyMapper final : public ROOT::TTreeProcessorMapper<std::tuple<int>, float> {
public:
  MyMapper(int starter_count) : count(starter_count) {}

  std::tuple<int> map(float) const noexcept __attribute__((always_inline)) {count++; return 1;}

  bool finalize() {std::cout << "There were " << count << "events.\n"; return true;}

private:
  // Hmm... this suppresses move-constructors.
  //mutable std::atomic<int> count;
  mutable int count;
};

struct Bar {};

template<class T>
using  proper_arg = typename std::conditional<std::is_move_constructible<T>::value, T&&, T&>::type;
template<class T>
using  proper_store = typename std::conditional<std::is_move_constructible<T>::value, T, T&>::type;

template<class... Test>
void foo( proper_arg<Test>... bar ) {
  std::tuple<proper_store<Test>...> baz = std::forward_as_tuple<proper_arg<Test>...>(std::move(bar)...);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr <<"Usage: " << argv[0] << " fname\n";
    return 1;
  }

  foo<MyMapper>( MyMapper(1) );
  //foo<Bar>( Bar() );

  //ROOT::TTreeProcessor<std::tuple<float>, MyMapper> processor({"a"}, MyMapper(argc));
  ROOT::TTreeProcessor<std::tuple<float>, MyMapper> processor({"a"}, MyMapper(argc));
  processor
    .map([](int) -> std::tuple<int> {return {1};})
    .process("T", {TFile::Open(argv[1])});

  return 0;
}
