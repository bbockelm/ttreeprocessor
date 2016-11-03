
#include "TTreeReader.h"

#include "Helpers.h"

namespace ROOT {

namespace internal {

template<typename BranchTypes, std::size_t... I>
struct reader_tuple_type {
    typedef std::tuple<TTreeReaderValue<typename std::tuple_element<I, BranchTypes>::type> ...> type;
};

template<typename BranchTypes, std::size_t... I>
typename reader_tuple_type<BranchTypes, I...>::type
make_reader_tuple_helper(TTreeReader &reader, typename internal::convert_to_strings<BranchTypes>::type branch_names) {
    return std::make_tuple(TTreeReaderValue<typename std::tuple_element<I, BranchTypes>::type>(reader, std::get<I>(branch_names)) ...);
}

template<typename BranchTypes>
typename reader_tuple_type<BranchTypes, std::make_index_sequence<std::tuple_size<BranchTypes>::value>{} >::type
make_reader_tuple(TTreeReader &reader, typename internal::convert_to_strings<BranchTypes>::type branch_names) {
    return make_reader_tuple_helper<BranchTypes, std::make_index_sequence<std::tuple_size<BranchTypes>::value>>(reader, branch_names);
}

template<typename BranchTypes, std::size_t... I>
BranchTypes
read_event_data_helper(typename reader_tuple_type<BranchTypes, std::make_index_sequence< std::tuple_size<BranchTypes>::value >{} >::type readers) {
    return std::make_tuple(*std::get<I>(readers)...);
}

template<typename BranchTypes>
BranchTypes
read_event_data(typename reader_tuple_type<BranchTypes, std::make_index_sequence<std::tuple_size<BranchTypes>::value>{} >::type readers) {
    return read_event_data_helper<BranchTypes, std::make_index_sequence<std::tuple_size<BranchTypes>::value>::type>(readers);
}

}  // namespace internal

}  // namespace ROOT

