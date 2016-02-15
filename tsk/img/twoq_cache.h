#pragma once

#include <list>
//#include <map>
//#include <set>
#include <utility>

#include <unordered_map>
#include <unordered_set>

template <class K, class V>
class TwoQCache {
public:
  typedef K key_type;
  typedef V value_type;

  TwoQCache(size_t max): N(max), kin(N/4), kout(N/2) {}

  std::pair<value_type&, bool> get(const key_type& key) {
    // if X is in Am then
    auto i = am_hash.find(key);
    if (i != am_hash.end()) {
      // move X to the head of Am
      am.splice(am.begin(), am, i->second);
      return std::pair<value_type&, bool>(am.front().second, true);
    }
    else {
      // else if X is in A1out then
      auto j = a1out_hash.find(key);
      if (j != a1out_hash.end()) {
        // relcaimfor(X)
        // add X to the head of Am
        am.push_front(reclaimfor());
        am.front().first = key;
        am_hash.emplace(key, am.begin());
        return std::pair<value_type&, bool>(am.front().second, false);
      }
      else {
        // else if X is in A1in then
        auto k = a1in_hash.find(key);
        if (k != a1in_hash.end()) {
          // do nothing
          return std::pair<value_type&, bool>(k->second->second, true);
        }
        else {
          // reclaimfor(X)
          // add X to the head of A1in
          a1in.push_front(reclaimfor());
          a1in.front().first = key;
          a1in_hash.emplace(key, a1in.begin());
          return std::pair<value_type&, bool>(a1in.front().second, false);
        }
      }
    }
  }

  const size_t size() const { return N; }

  void clear() {
    // TODO
  }

private:
   std::pair<key_type, value_type> reclaimfor() {
    // if there are free page slots then
    if (am_hash.size() + a1in_hash.size() < N) {
      // put X into a free page slot
      return std::pair<key_type, value_type>(K(), V());   
    }
    // else if |A1in| > Kin
    else if (a1in_hash.size() > kin) {
      // page out the tail of A1in, call it Y
      // add identifier Y to the head of A1out
      a1out.push_front(a1in.back().first);
      a1out_hash.insert(a1in.back().first);
      a1in_hash.erase(a1in.back().first);

      // if |A1out| > Kout
      if (a1out.size() > kout) {
        // remove identifier of Z from the tail of A1out
        a1out_hash.erase(a1out.back());
        a1out.pop_back();
      }

      // put X into the reclaimed page slot
      auto x = std::move(a1in.back());
      a1in.pop_back();
      return std::move(x);
    }
    else {
      // page out the tail of Am, call it Y
      // put X into the reclaimed page slot
      am_hash.erase(am.back().first);
      auto x = std::move(am.back());
      am.pop_back();
      return std::move(x); 
    }
  }

// TODO: try unordered_map, unordered_set

  const size_t N, kin, kout;
  std::list<std::pair<key_type, value_type>> a1in, am;
/*
  std::map<key_type, typename decltype(am)::iterator> am_hash;
  std::map<key_type, typename decltype(a1in)::iterator> a1in_hash;
  std::set<key_type> a1out_hash;
*/
  std::unordered_map<key_type, typename decltype(am)::iterator> am_hash;
  std::unordered_map<key_type, typename decltype(a1in)::iterator> a1in_hash;
  std::unordered_set<key_type> a1out_hash;
  std::list<key_type> a1out;
};
