
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

class FilterOne : ROOT::TTreeFilter {
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

// Test the case where the filter is in the first position.
static_assert(std::is_same<ProcessorArgHelper<0, 0, std::tuple<float, float>, FilterOne, MapOne, MapTwo>::input_type, std::tuple<float, float>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 0, std::tuple<float, float>, FilterOne, MapOne, MapTwo>::output_type, std::tuple<float, float>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, FilterOne, MapOne, MapTwo, MapThree>::input_type, std::tuple<float, float>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, FilterOne, MapOne, MapTwo, MapThree>::output_type, std::tuple<int, int>>::value, "");
// Test case where filter appears in the middle of the chain.
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, MapOne, FilterOne, MapTwo, MapThree>::input_type, std::tuple<int, int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 1, std::tuple<float, float>, MapOne, FilterOne, MapTwo, MapThree>::output_type, std::tuple<int, int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<float, float>, MapOne, FilterOne, MapTwo, MapThree>::input_type, std::tuple<int, int>>::value, "");
static_assert(std::is_same<ProcessorArgHelper<0, 2, std::tuple<float, float>, MapOne, FilterOne, MapTwo, MapThree>::output_type, std::tuple<double, double>>::value, "");

// Make sure GetStageType correctly evalutes stage types.
static_assert(GetStageType<0, MapOne, FilterOne, MapTwo>::value == 1, "");
static_assert(GetStageType<1, MapOne, FilterOne, MapTwo>::value == 0, "");
static_assert(GetStageType<2, MapOne, FilterOne, MapTwo>::value == 1, "");
static_assert(GetStageType<0, MapOne, FilterOne>::value == 1, "");
static_assert(GetStageType<1, MapOne, FilterOne>::value == 0, "");
static_assert(GetStageType<0, MapOne>::value == 1, "");
static_assert(GetStageType<0, FilterOne>::value == 0, "");
static_assert(GetStageType<1, MapOne, MapTwo>::value == 1, "");
static_assert(GetStageType<1, MapOne, FilterOne>::value == 0, "");
static_assert(GetStageType<2, MapOne, MapTwo, FilterOne>::value == 0, "");
static_assert(GetStageType<2, MapOne, MapTwo, MapThree>::value == 1, "");

//static_assert(std::is_same<ProcessorResult<std::tuple<float, float>, MapOne, FilterOne, MapTwo>::output_type, std::tuple<double, double>>::value, "");
static_assert(std::is_same<ProcessorResult<std::tuple<float, float>, MapOne, MapTwo, MapThree, MapFive>::output_type, std::tuple<int>>::value, "");
//static_assert(std::is_same<ProcessorResult<std::tuple<float, float>, MapOne, FilterOne, MapTwo, MapThree>::output_type, std::tuple<int>>::value, "");

int main(int argc, char *argv[]) {
  return 0;
}

