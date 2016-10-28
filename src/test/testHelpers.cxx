
#include "Helpers.h"

using namespace ROOT::internal;

class MapOne {
  public:
    std::tuple<int, int> operator()(float, float) {}
    std::tuple<int, int> map(float, float) {}
};

class MapTwo {
  public:
    std::tuple<double, double> map(int, int) {}
};

class MapThree {
  public:
    int map(double, double) {}
};

class MapFour {
  public:
    int map(float) {}
};

static_assert(std::is_same<std::result_of<decltype(&MapOne::map)(MapOne, float, float)>::type, std::tuple<int, int>>::value, "");


static_assert(std::is_same< result_of_unpacked_tuple<0, 2, MapOne, std::tuple<float, float>>::type, std::tuple<int, int> >::value, "");
static_assert(std::is_same< result_of_unpacked_tuple<0, 2, MapThree, std::tuple<double, double>>::type, int >::value, "");

static_assert(std::is_same<ProcessorArgHelper<0, 0, std::tuple<float, float>, MapOne, MapTwo, MapThree>::input_type, std::tuple<float, float>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 0, std::tuple<float, float>, MapOne, MapTwo, MapThree>::output_type, std::tuple<int, int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, MapOne, MapTwo, MapThree>::input_type, std::tuple<int, int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, MapOne, MapTwo, MapThree>::output_type, std::tuple<double, double>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<float, float>, MapOne, MapTwo, MapThree>::input_type, std::tuple<double, double>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<float, float>, MapOne, MapTwo, MapThree>::output_type, int>::value, "");

int main(int argc, char *argv[]) {
  return 0;
}

