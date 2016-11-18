#ifndef __TTREE_PROCESSOR_KERNELS_H_
#define __TTREE_PROCESSOR_KERNELS_H_

namespace ROOT {

namespace internal {
/**
 * The base class for mappers.
 *
 * This empty base is used to determine when things derive from the filter:
 * the actual basic implementation is found in TTreeProcessorMapper, but
 * that's tedious to use in type_traits as it is a template.
 *
 * Users should not derive from this directly; instead derive from
 * TTreeProcessorMapper
 */
class TTreeProcessorMapperBase {};

/**
 * The base class for filters.
 *
 * This empty base is used to determine when things derive from the filter:
 * the actual basic implementation is found in TTreeProcessorMapper, but
 * that's tedious to use in type_traits as it is a template.
 *
 * Users shold not derive from this directly; instead derive from
 * TTreeProcessorFilter.
 */
class TTreeProcessorFilterBase {};

}  // internal

/**
 * The base implementation of mappers.
 *
 * Note that `map` is invoked as const; that means any access
 * to a member variable must be THREAD SAFE!
 */
template<typename T, typename... InputArgs>
class TTreeProcessorMapper : public internal::TTreeProcessorMapperBase {
  public:
    TTreeProcessorMapper() {}
    TTreeProcessorMapper(const TTreeProcessorMapper&) = delete;
    TTreeProcessorMapper(TTreeProcessorMapper&&) = default;

    T map (InputArgs...) const noexcept {};

    bool finalize() {return true;}

    typedef T output_type;
};

/**
 * Base implementation of a filter.
 */
template<typename... InputArgs>
class TTreeProcessorFilter : public internal::TTreeProcessorFilterBase {
  public:
    TTreeProcessorFilter() {}
    TTreeProcessorFilter(const TTreeProcessorFilter&) = delete;
    TTreeProcessorFilter(TTreeProcessorFilter&&) = default;

    bool filter(InputArgs...) const noexcept {};

    bool finalize() {return true;}
};

}  // ROOT

#endif

