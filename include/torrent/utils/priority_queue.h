// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2007, Jari Sundell <jaris@ifi.uio.no>

// priority_queue is a priority queue implemented using a binary
// heap. It can contain multiple instances of a value.

#ifndef LIBTORRENT_UTILS_PRIORITY_QUEUE_H
#define LIBTORRENT_UTILS_PRIORITY_QUEUE_H

#include <algorithm>
#include <vector>

namespace torrent {
namespace utils {

template<typename Value,
         typename Compare,
         typename Equal,
         typename Alloc = std::allocator<Value>>
class priority_queue : public std::vector<Value, Alloc> {
public:
  using base_type       = std::vector<Value, Alloc>;
  using reference       = typename base_type::reference;
  using const_reference = typename base_type::const_reference;
  using iterator        = typename base_type::iterator;
  using const_iterator  = typename base_type::const_iterator;
  using value_type      = typename base_type::value_type;

  using base_type::begin;
  using base_type::clear;
  using base_type::empty;
  using base_type::end;
  using base_type::size;

  priority_queue(Compare l = Compare(), Equal e = Equal())
    : m_compare(l)
    , m_equal(e) {}

  const_reference top() const {
    return base_type::front();
  }

  void make_heap() {
    std::make_heap(begin(), end(), m_compare);
  }

  void pop() {
    std::pop_heap(begin(), end(), m_compare);
    base_type::pop_back();
  }

  void push(const value_type& value) {
    base_type::push_back(value);
    std::push_heap(begin(), end(), m_compare);
  }

  template<typename Key>
  iterator find(const Key& key) {
    return std::find_if(begin(), end(), [this, key](Key& curKey) {
      return m_equal(curKey, key);
    });
  }

  template<typename Key>
  bool erase(const Key& key) {
    iterator itr = find(key);

    if (itr == end())
      return false;

    erase(itr);
    return true;
  }

  // Removes 'itr' from the queue.
  void erase(iterator itr) {
    base_type::erase(itr);
    std::make_heap(begin(), end(), m_compare);
  }

private:
  Compare m_compare;
  Equal   m_equal;
};

} // namespace utils
} // namespace torrent

#endif
