
#include <memory>

#include "TFile.h"
#include "TTreeReader.h"

#include "VcHelpers.h"
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

// Read a single-event at a time; non-vectorized mode.
template<typename BranchTypes, typename ReaderType, std::size_t... I>
BranchTypes
read_event_data_helper(ReaderType& readers, std::index_sequence<I...>) {
    return std::make_tuple(*(*std::get<I>(readers))...);
}

template<unsigned int IsVectorized, typename BranchTypes, typename ReaderType, typename ReaderValueType>
class read_event_data;

template<typename BranchTypes, typename ReaderType, typename ReaderValueType>
class read_event_data<0, BranchTypes, ReaderType, ReaderValueType> {
  public:

    BranchTypes operator()(ReaderType&, ReaderValueType& readers) {
      return read_event_data_helper<BranchTypes>(readers, std::make_index_sequence< std::tuple_size<BranchTypes>::value >());
    }
};

template<typename LHS, typename RHS>
bool param_pack_assign (LHS &lhs, const RHS &rhs)
{
  lhs = rhs;
  return false;
}

template<typename LHS, typename RHS>
bool param_pack_load (LHS &lhs, const RHS &rhs)
{
  lhs.load(rhs);
  return false;
}

// Read into a Vc vector type.
template<typename BranchTypes, typename ReaderType, typename ReaderValueType, std::size_t... I>
vectorized_tuple_t<BranchTypes>
read_event_data_vectorized_helper(ReaderType& reader, ReaderValueType& readerValues, std::index_sequence<I...>)
{
    int idx;

    std::tuple<std::array<bool, vector_count>,
               std::array<typename std::tuple_element<I, ReaderValueType>::type::element_type::NonConstT_t, vector_count>...
              > dataPrep;
    auto &maskPrep = std::get<0>(dataPrep);
    // Initialize the event data from the TTreeReader.
    bool isValid = true;
    for (idx=0; idx<vector_count && isValid; idx++) {
        maskPrep[idx] = 1;
        bool ignore_array[] = { param_pack_assign(std::get<I+1>(dataPrep)[idx], *(*std::get<I>(readerValues)))... };
        (void) ignore_array;
        isValid = (idx+1<vector_count) && reader.Next();
    }

    // Set the remainder of the mask to 0.
    for (; idx < vector_count; idx++) {
        maskPrep[idx] = 0;
    }

    // Initialize the vectorized tuple from the std::arrays.
    std::tuple<maskv, vector_t<typename std::tuple_element<I, ReaderValueType>::type::element_type::NonConstT_t>...> data;
    std::get<0>(data).load(std::get<0>(dataPrep).data());
    bool ignore_array[] = { param_pack_load(std::get<I+1>(data), std::get<I+1>(dataPrep).data())... };
    (void) ignore_array;

    return data;
}

template<typename BranchTypes, typename ReaderType, typename ReaderValueType>
class read_event_data<1, BranchTypes, ReaderType, ReaderValueType> {
  public:

    vectorized_tuple_t<BranchTypes> operator()(ReaderType& reader, ReaderValueType& readerValues) {
      return read_event_data_vectorized_helper<BranchTypes>(reader, readerValues, std::make_index_sequence< std::tuple_size<BranchTypes>::value >());
    }
};

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

