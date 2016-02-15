#pragma once

#include <list>
#include <map>
#include <memory>

template <class K, class V>
class LIRSCache {
public:
  typedef K key_type;
  typedef V value_type;

  LIRSCache(size_t max): N(max), llirs(N-N/100), lhirs(N/100) {}

  std::pair<value_type&, bool> get(const key_type& key) {
    // X is LIR  <=> X in S, X not in Q, and X resident
    // X is HIR  <=> X is not LIR
    // X is in Q <=> X is HIR and resident

    auto i = s_hash.find(key);
    if (s_hash.size() < llirs) {
      // S is not yet full, make X LIR
      if (i != s_hash.end()) {
        // X is in S
        // move X to head of S
        s.splice(s.begin(), s, i->second);
        return std::pair<value_type&, bool>(*s.front().second, true);
      }
      else {
        // X is not in S
        // add X to head of S
        s.emplace_front(key, std::make_shared<V>());
        s_hash.emplace(key, s.begin());
        return std::pair<value_type&, bool>(*s.front().second, false);
      }
    }

    auto j = q_hash.find(key);

    if (j != q_hash.end()) {
      // => X is in Q => X is HIR and resident
      if (i != s_hash.end()) {
        // => X is in S

        // promote X to LIR
        // move X to head of S
        s.splice(s.begin(), q, j->second);
        s_hash.emplace(key, s.begin());
        q_hash.erase(j);

        // move LIR item at tail of S to tail of Q
        // XXX: why must item at tail of S be LIR?
        
        if (q_hash.size() == lhirs) {
          q_hash.erase(q.back().first);
          q.pop_back();
        }

        s_hash.erase(s.back().first);
        q.splice(q.end(), s, std::prev(s.end()));
        q_hash.emplace(q.back().first, std::prev(q.end()));         

        // prune S
        prune();

        return std::pair<value_type&, bool>(*s.front().second, true);
      }
      else {
        // => X is not in S
        // X remains HIR
        // move X to tail of Q
        q.splice(q.end(), q, j->second);
        return std::pair<value_type&, bool>(*q.back().second, true);
      }
    }
    else {
      // => X is not in Q
      if (i != s_hash.end() && i->second->second) {
        // => X is in S and X is resident => X is LIR

        // check if X is at tail of S
        const bool at_tail = i->second == std::prev(s.end());
        // move X to head of S
        s.splice(s.begin(), s, i->second);
        // prune S if X had been at tail 
        if (at_tail) {
          prune();
        }

        return std::pair<value_type&, bool>(*s.front().second, true);
      }
      else {
        // => X is nonresident => X is HIR

        // remove head of Q, make it nonresident if in S
        if (!q.empty()) {
          auto k = s_hash.find(q.front().first);
          if (k != s_hash.end()) {
            k->second->second.reset(); 
          }
          q_hash.erase(q.front().first);
          q.pop_front();
        }

        if (i != s_hash.end()) {
          // => X is in S
          // promote X to LIR

          // move X to head of S
          // TODO: steal value from newly nonres item?
          s.splice(s.begin(), s, i->second);
          // make X resident
          s.front().second.reset(new V());

          // demote LIR block at tail of S to tail of Q
          // XXX: why must tail of S be LIR?
          q.splice(q.end(), s, std::prev(s.end()));
          q_hash.emplace(q.back().first, std::prev(q.end()));
          s_hash.erase(q.back().first);
         
          // prune S 
          prune();
        }
        else {
          // => X is not in S => X is nonresident with infinite HIR

          // add X to head of S
          
          if (s_hash.size() == llirs) {
            s_hash.erase(s.back().first);
            s.pop_back();
          }

          s.emplace_front(key, std::make_shared<V>());
          s_hash.emplace(key, s.begin());

          // add X to tail of Q
          
          if (q_hash.size() == lhirs) {
            q_hash.erase(q.back().first);
            q.pop_back();
          }

          q.emplace_back(key, s.front().second);
          q_hash.emplace(key, std::prev(q.end())); 
        }

        return std::pair<value_type&, bool>(*s.front().second, false);
      }
    }
  }

private:
  void prune() {
    // remove blocks from the tail of S until the tail is LIR
    // => remove tail blocks which are nonresident or in Q
    while (!s.empty() && (!s.back().second || q_hash.find(s.back().first) != q_hash.end())) {
      s_hash.erase(s.back().first);
      s.pop_back();
    }
  }

  const size_t N, llirs, lhirs;

  std::list<std::pair<key_type, std::shared_ptr<value_type>>> s, q;
  std::map<const key_type, typename decltype(s)::iterator> s_hash;
  std::map<const key_type, typename decltype(q)::iterator> q_hash;
};
