
#include <memory>

#include "TFile.h"
#include "TTreeReader.h"

#include "Helpers.h"

namespace ROOT {

namespace internal {

template<typename BranchTypes, std::size_t... I>
struct reader_tuple_type {
    typedef std::tuple<std::shared_ptr<TTreeReaderValue<typename std::tuple_element<I, BranchTypes>::type>> ...> type;
};

template<typename BranchTypes, std::size_t... I>
typename reader_tuple_type<BranchTypes, I...>::type
make_reader_tuple_helper(TTreeReader &reader, typename internal::convert_to_strings<BranchTypes>::type &branch_names, std::index_sequence<I...>) {
    return std::make_tuple(std::make_shared<TTreeReaderValue<typename std::tuple_element<I, BranchTypes>::type>>(reader, std::get<I>(branch_names).c_str()) ...);
}

template<typename BranchTypes>
auto
make_reader_tuple(TTreeReader &reader, typename internal::convert_to_strings<BranchTypes>::type &branch_names) {
    return make_reader_tuple_helper<BranchTypes>(reader, branch_names, std::make_index_sequence< std::tuple_size<BranchTypes>::value >());
}

template<typename BranchTypes, typename ReaderType, std::size_t... I>
BranchTypes
read_event_data_helper(ReaderType& readers, std::index_sequence<I...>) {
    return std::make_tuple(*(*std::get<I>(readers))...);
}

template<typename BranchTypes, typename ReaderType>
BranchTypes
read_event_data(ReaderType& readers) {
    return read_event_data_helper<BranchTypes>(readers, std::make_index_sequence< std::tuple_size<BranchTypes>::value >());
}

// Helper to generate a valid TFile
class TFileHelper {
public:
    TFileHelper(const std::string &fname) : m_tf(TFile::Open(fname.c_str())) {}

    TFile *get() {return m_tf;}

private:
    TFile *m_tf{nullptr};
};

}  // namespace internal

}  // namespace ROOT

