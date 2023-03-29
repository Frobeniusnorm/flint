#include "flint.h"
#include <array>
#include <string>
#include <tuple>
#include <vector>
/**
 * Useful helper functions used by the library itself.
 */
namespace FLINT_HPP_HELPER {
/**
 * Transforms a vector of arbitrary recursive dimensions to a string
 */
template <typename T>
static inline std::string vectorString(const std::vector<T> &vec,
                                       std::string indentation = "") {
  std::string res = "[";
  for (size_t i = 0; i < vec.size(); i++) {
    res += std::to_string(vec[i]);
    if (i != vec.size() - 1)
      res += ", ";
  }
  return res + "]";
}
template <typename T>
static inline std::string vectorString(const std::vector<std::vector<T>> &vec,
                                       std::string indentation = "") {
  std::string res = "[";
  for (size_t i = 0; i < vec.size(); i++) {
    res += vectorString(vec[i], indentation + " ");
    if (i != vec.size() - 1)
      res += ",\n" + indentation;
  }
  return res + "]";
}
/**
 * Transforms an array of arbitrary recursive dimensions to a string
 */
template <typename T, size_t n>
static inline std::string arrayString(const std::array<T, n> &vec) {
  std::string res = "[";
  for (size_t i = 0; i < n; i++) {
    res += std::to_string(vec[i]);
    if (i != vec.size() - 1)
      res += ", ";
  }
  return res + "]";
}
template <typename T, size_t n, size_t k>
static inline std::string
arrayString(const std::array<std::array<T, k>, n> &vec) {
  std::string res = "[";
  for (size_t i = 0; i < n; i++) {
    res += arrayString(vec[i]);
    if (i != n - 1)
      res += ",\n";
  }
  return res + "]";
}
template <typename E, typename T>
static std::vector<E> flattened(const std::vector<std::vector<T>> vec) {
  using namespace std;
  vector<T> result;
  for (const vector<T> &v : vec) {
    result.insert(result.end(), v.begin(), v.end());
  }
  return result;
}

template <typename E, typename T>
static std::vector<E>
flattened(const std::vector<std::vector<std::vector<T>>> vec) {
  using namespace std;
  vector<E> result;
  for (const vector<vector<T>> &v : vec) {
    vector<E> rec = flattened<E>(v);
    result.insert(result.end(), rec.begin(), rec.end());
  }
  return result;
}
template <typename E, typename T>
static std::vector<E>
flattened(const std::initializer_list<std::initializer_list<T>> vec) {
  using namespace std;
  vector<T> result;
  for (const initializer_list<T> &v : vec) {
    result.insert(result.end(), v.begin(), v.end());
  }
  return result;
}

template <typename E, typename T>
static std::vector<E> flattened(
    const std::initializer_list<std::initializer_list<std::initializer_list<T>>>
        vec) {
  using namespace std;
  vector<E> result;
  for (const initializer_list<initializer_list<T>> &v : vec) {
    vector<E> rec = flattened<E>(v);
    result.insert(result.end(), rec.begin(), rec.end());
  }
  return result;
}
}; // namespace FLINT_HPP_HELPER
// checks if the given type is one of the allowed tensor types
template <typename T> static constexpr void isTensorType() {
  static_assert(std::is_same<T, int>() || std::is_same<T, float>() ||
                    std::is_same<T, long>() || std::is_same<T, double>(),
                "Only integer and floating-point Tensor types are allowed");
}
// converts c++ type to flint type
template <typename T> static constexpr FType toFlintType() {
  if (std::is_same<T, int>())
    return F_INT32;
  if (std::is_same<T, long>())
    return F_INT64;
  if (std::is_same<T, float>())
    return F_FLOAT32;
  else
    return F_FLOAT64;
}
// checks which of both types the flint backend will choose
template <typename K, typename V> static constexpr bool isStronger() {
  const int a = std::is_same<K, int>()     ? 0
                : std::is_same<K, long>()  ? 1
                : std::is_same<K, float>() ? 2
                                           : 3;
  const int b = std::is_same<V, int>()     ? 0
                : std::is_same<V, long>()  ? 1
                : std::is_same<V, float>() ? 2
                                           : 3;
  return a >= b;
}
template <typename K> static constexpr bool isInt() {
  return std::is_same<K, int>() || std::is_same<K, long>();
}
template <typename T>
using to_float = typename std::conditional<isInt<T>(), double, T>::type;
/**
 * Contains static methods to configure Flints behaviour-
 */
namespace Flint {
/** Sets the Logging Level of the Flint Backend */
inline void setLoggingLevel(int level) { fSetLoggingLevel(level); }
}; // namespace Flint
/**
 * Encapsulates the data of a tensor. Is only valid as long as the Tensor is
 * valid. Provides an interface for index operations on multidimensional data.
 */
template <typename T, unsigned int dimensions> class TensorView;
template <typename T> class TensorView<T, 1> {
  T *data;
  const size_t already_indexed;
  const size_t shape;

public:
  TensorView(T *data, const std::vector<size_t> shape,
             const size_t already_indexed)
      : data(data), already_indexed(already_indexed), shape(shape[0]) {}
  /**
   * Returns a read-write-reference to the index data entry of the Tensor-data.
   * Only valid as long as the original Tensor is valid.
   */
  T &operator[](size_t index) { return data[already_indexed + index]; }
  size_t size() const { return shape; }
};
template <typename T, unsigned int n> class TensorView {
  T *data;
  const size_t already_indexed;
  const std::vector<size_t> shape;

public:
  TensorView(T *data, const std::vector<size_t> shape,
             const size_t already_indexed)
      : data(data), already_indexed(already_indexed), shape(shape) {}
  /**
   * Returns a new TensorView object with one more index for the current
   * dimension (i.e. the new TensorView has one dimension less). Only valid as
   * long as the original Tensor is valid.
   */
  TensorView<T, n - 1> operator[](size_t index) {
    std::vector<size_t> ns(shape.size() - 1);
    for (size_t i = 0; i < shape.size() - 1; i++) {
      ns[i] = shape[i + 1];
      index *= shape[i + 1];
    }
    return TensorView<T, n - 1>(data, ns, already_indexed + index);
  }
};
/**
 * Describes a slice operation for one dimension.
 */
struct TensorRange {
  static const long MAX_SIZE = 2147483647;
  long start = 0;
  long end = MAX_SIZE;
  long step = 1;
  TensorRange() = default;
  TensorRange(std::tuple<long, long, long> range_vals)
      : start(std::get<0>(range_vals)), end(std::get<1>(range_vals)),
        step(std::get<2>(range_vals)) {}
  TensorRange(std::initializer_list<long> range_vals) {
    if (range_vals.size() > 0)
      start = *range_vals.begin();
    if (range_vals.size() > 1)
      end = *(range_vals.begin() + 1);
    if (range_vals.size() > 2)
      step = *(range_vals.begin() + 2);
  }
  TensorRange(long start, long end = MAX_SIZE, long step = 1)
      : start(start), end(end), step(step) {}
};