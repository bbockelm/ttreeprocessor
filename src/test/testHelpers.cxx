
#include "Helpers.h"

using namespace ROOT::internal;

class MapOne : ROOT::TTreeMapper {
  public:
    std::tuple<int, int> operator()(float, float) {}
    std::tuple<int, int> map(float, float) {}
};

class MapTwo : ROOT::TTreeMapper {
  public:
    std::tuple<double, double> map(int, int) {}
};

class MapThree : ROOT::TTreeMapper {
  public:
    std::tuple<int> map(double, double) {}
};

class MapFour : ROOT::TTreeMapper {
  public:
    int map(float) {}
};

class MapFive : ROOT::TTreeMapper {
  public:
    std::tuple<int> map(int);
};

static_assert(std::is_same<std::result_of<decltype(&MapOne::map)(MapOne, float, float)>::type, std::tuple<int, int>>::value, "");


static_assert(std::is_same< result_of_unpacked_tuple<MapOne, std::tuple<float, float>>::type, std::tuple<int, int> >::value, "");
static_assert(std::is_same< result_of_unpacked_tuple<MapThree, std::tuple<double, double>>::type, std::tuple<int> >::value, "");

static_assert(std::is_same<ProcessorArgHelper<0, 0, std::tuple<float, float>, MapOne, MapTwo, MapThree>::input_type, std::tuple<float, float>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 0, std::tuple<float, float>, MapOne, MapTwo, MapThree>::output_type, std::tuple<int, int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, MapOne, MapTwo, MapThree>::input_type, std::tuple<int, int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, MapOne, MapTwo, MapThree>::output_type, std::tuple<double, double>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<float, float>, MapOne, MapTwo, MapThree>::input_type, std::tuple<double, double>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<float, float>, MapOne, MapTwo, MapThree>::output_type, std::tuple<int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<double, double>, MapTwo, MapThree, MapFive>::input_type, std::tuple<int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<double, double>, MapTwo, MapThree, MapFive>::output_type, std::tuple<int>>::value, "");

int main(int argc, char *argv[]) {
  return 0;
}

