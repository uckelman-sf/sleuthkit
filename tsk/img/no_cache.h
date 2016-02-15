#pragma once

#include <utility>

template <class K, class V>
class NoCache {
public:
  typedef K key_type;
  typedef V value_type;

  std::pair<value_type&, bool> get(const key_type& key) {
    return std::pair<value_type&, bool>(val, false);
  }

  constexpr size_t size() const { return 0; }

  void clear() {}

private:
  value_type val;
};
