#pragma once

#include <list>
#include <unordered_map>
#include <utility>

template <class K, class V>
class LRUCache {
public:
  typedef K key_type;
  typedef V value_type;

  LRUCache(size_t max): N(max) {}

  std::pair<value_type&, bool> get(const key_type& key) {
    // try adding new key to hash
    auto r = hash.emplace(key, items.end());
    if (r.second) {
      // new key inserted
      if (items.size() < N) {
        // allocate a new item
        items.emplace_front(key, V());
      }
      else {
        // reuse LRU item
        items.splice(items.begin(), items, std::prev(items.end()));
        if (items.front().first != key) {
          // remove the key from the reused LRU item
          hash.erase(items.front().first);
        }
      }
      (r.first->second = items.begin())->first = key;
    }
    else {
      // found existing key, make its item MRU
      items.splice(items.begin(), items, r.first->second);
    }

    // return the item, indicating preexistance
    return std::pair<value_type&, bool>(items.front().second, !r.second);
  }

  size_t size() const { return N; }

  void clear() {
    hash.clear();
  }

  typename std::list<std::pair<key_type, value_type>>::const_iterator begin() const {
    return items.cbegin();
  }

  typename std::list<std::pair<key_type, value_type>>::const_iterator end() const {
    return items.cend();
  }

private:
  const size_t N;

  std::list<std::pair<key_type, value_type>> items;
  std::unordered_map<key_type, typename decltype(items)::iterator> hash;
};
