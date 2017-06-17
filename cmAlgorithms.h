/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmAlgorithms_h
#define cmAlgorithms_h

#include <algorithm>
#include <utility>

namespace ContainerAlgorithms {

template<typename T>
struct cmIsPair
{
  enum
  {
    value = false
  };
};

template<typename K, typename V>
struct cmIsPair<std::pair<K, V>>
{
  enum
  {
    value = true
  };
};

template<
  typename Range,
  bool valueTypeIsPair = cmIsPair<typename Range::value_type>::value>
struct DefaultDeleter
{
  void operator()(typename Range::value_type value) const { delete value; }
};

template<typename Range>
struct DefaultDeleter<Range, /* valueTypeIsPair = */ true>
{
  void operator()(typename Range::value_type value) const
  {
    delete value.second;
  }
};

} // namespace ContainerAlgorithms

template<typename Range>
void cmDeleteAll(Range const& r)
{
  std::for_each(
    r.begin(), r.end(), ContainerAlgorithms::DefaultDeleter<Range>());
}

#endif
