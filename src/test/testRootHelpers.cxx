
#include "RootHelpers.h"

#include "TFile.h"

typedef std::tuple<int, float> MyBranchTypes;

static_assert( std::is_same< std::tuple<std::shared_ptr<TTreeReaderValue<int>>, std::shared_ptr<TTreeReaderValue<float>>>,
                             ROOT::internal::reader_tuple_type<MyBranchTypes, 0, 1>::type
                           >::value,
               "");

int main(int argc, char *argv[]) {

  return 0;
}

