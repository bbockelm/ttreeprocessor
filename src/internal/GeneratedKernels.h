#ifndef __GENERATED_KERNELS_H_
#define __GENERATED_KERNELS_H_

/*
 * Various kernel classes that are generated and used internally by the
 * TTreeProcessor: not meant to be used directly by users.
 *
 * For example, the TTreeProcessorMapperLambda template takes a lambda
 * function and generates corresponding a Mapper class that can be used
 * by the TTreeProcessor
 */

namespace ROOT {

namespace internal {

/**
 * Definition of a mapper derived from a user-provided lambda function.
 */
template<unsigned int IsVectorized, typename T, typename... InputArgs>
class TTreeProcessorMapperLambda;

template<typename T, typename... InputArgs>
class TTreeProcessorMapperLambda<0, T, InputArgs...> final : public TTreeProcessorMapper<typename std::result_of<T(InputArgs...)>::type, InputArgs...> {
  public:
    TTreeProcessorMapperLambda(const T& fn) : m_fn(fn) {}

    typename std::result_of<T(InputArgs...)>::type map (InputArgs ...args) const noexcept {
      return m_fn(args...);
    }

  private:
    T m_fn;
};

template<typename T, typename... InputArgs>
class TTreeProcessorMapperLambda<1, T, InputArgs...> final : public TTreeProcessorMapper<typename std::result_of<T(InputArgs...)>::type, InputArgs...> {
  public:
    TTreeProcessorMapperLambda(const T& fn) : m_fn(fn) {}

    typename std::result_of<T(InputArgs...)>::type map (InputArgs ...args) const noexcept {
      return m_fn(args...);
    }

  private:
    T m_fn;
};

/**
 * Filters derived from a user-provided lambda.
 */
template<unsigned int IsVectorized, typename T, typename... InputArgs>
class TTreeProcessorFilterLambda;

template<typename T, typename... InputArgs>
class TTreeProcessorFilterLambda<0, T, InputArgs...> final : public TTreeProcessorFilter<InputArgs...> {
  public:
    TTreeProcessorFilterLambda(const T& fn) : m_fn(fn) {}

    bool filter(InputArgs ...args) const noexcept {
      return m_fn(args...);
    }

  private:
    T m_fn;
};

template<typename T, typename... InputArgs>
class TTreeProcessorFilterLambda<1, T, InputArgs...> final : public TTreeProcessorFilter<internal::vector_t<InputArgs>...> {
  public:
    TTreeProcessorFilterLambda(const T& fn) : m_fn(fn) {}

    bool filter(internal::vector_t<InputArgs> ...args) const noexcept {
      return m_fn(args...);
    }

  private:
    T m_fn;
};


/**
 * Counts the number of events seen by a mapper.
 * Prints the final number out at the end.
 */
template<typename... InputArgs>
class TTreeProcessorCountPrinter final : public TTreeProcessorMapper<std::tuple<InputArgs...>, InputArgs...> {
  typedef tbb::enumerable_thread_specific<int> EventCounter;

  public:
    TTreeProcessorCountPrinter() : m_counter(0) {
    }

    TTreeProcessorCountPrinter(TTreeProcessorCountPrinter && rhs) {
      m_counter = std::move(rhs.m_counter);
    }

    TTreeProcessorCountPrinter(const TTreeProcessorCountPrinter & rhs) {
      int sum = std::accumulate(rhs.m_counter.begin(), rhs.m_counter.end(), 0);
      m_counter = EventCounter(sum);
    }

    std::tuple<InputArgs...> map (InputArgs... args) const noexcept {
      //EventCounter::reference my_counter = m_counter->local();
      //++m_counter->local();
      m_counter.local()++;

      return std::make_tuple(args...);
    }

    bool finalize() {
      int sum = std::accumulate(m_counter.begin(), m_counter.end(), 0);
      //int sum = m_counter->combine([](int x, int y) {return x+y;});
      std::cout << "Counter saw " << sum << " events.\n";
    }

  private:
    mutable EventCounter m_counter;
};

}  // internal

}  // ROOT

#endif
