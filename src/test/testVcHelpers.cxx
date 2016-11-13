
#include "VcHelpers.h"

using namespace ROOT;
using namespace ROOT::internal;

class VectorMap1 : public TTreeMapper {
  public:
    int map(maskv, floatv) {return 0;}
};

class VectorMap2 : public TTreeMapper
{
  public:
    int map(floatv) {return 0;}
};

class NonVectorMap1 : public TTreeMapper {
  public:
    int map(float) {return 0;}
};

class OverloadedVectorMap1 : public TTreeMapper {
  public:
    int map(float) {return 1;}
    int map(maskv, floatv) {return 0;}
};

class VectorFilter : public TTreeFilter {
  public:
    int filter(maskv, floatv) {return 0;}
};

static_assert(is_vectorized<VectorMap1, std::tuple<float>>::value, "Did not recognize vectorized function.");
static_assert(is_vectorized<VectorFilter, std::tuple<float>>::value, "Did not recognize vectorized filter function.");
static_assert(!is_vectorized<NonVectorMap1, std::tuple<float>>::value, "Thought non-vectorized function was vectorized.");
static_assert(!is_vectorized<VectorMap2, std::tuple<float>>::value, "Thought non-vectorized function was vectorized.");
//static_assert(is_vectorized<OverloadedVectorMap1, std::tuple<float>>::value, "Did not recognize vectorized overloaded function.");

static_assert(std::is_same< input_tuple_t<std::tuple<float>, VectorMap1>, std::tuple<maskv, floatv> >::value, "Deduced wrong type");
static_assert(std::is_same< input_tuple_t<std::tuple<float>, VectorMap1, VectorFilter>, std::tuple<maskv, floatv> >::value, "Deduced wrong type");
static_assert(std::is_same< input_tuple_t<std::tuple<float>, VectorFilter>, std::tuple<maskv, floatv> >::value, "Deduced wrong type");
static_assert(std::is_same< input_tuple_t<std::tuple<float>, VectorMap2>, std::tuple<float> >::value, "Deduced wrong type");
//static_assert(std::is_same< input_tuple_t<std::tuple<float>, OverloadedVectorMap1>, std::tuple<maskv, floatv> >::value, "Deduced wrong type");
static_assert(std::is_same< input_tuple_t<std::tuple<float>, OverloadedVectorMap1>, std::tuple<float> >::value, "Deduced wrong type");
static_assert(std::is_same< input_tuple_t<std::tuple<float>>, std::tuple<float> >::value, "Deduced wrong type");
//static_assert(std::is_same< input_tuple_t<std::tuple<float>, OverloadedVectorMap1>, std::tuple<maskv, floatv> >::value, "Deduced wrong type");

int main(int argc, char *argv[]) {return 0;}

