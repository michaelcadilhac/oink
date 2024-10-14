#pragma once

#include <boost/heap/pairing_heap.hpp>

#include <vector>

namespace detail {
  enum class mutable_priority_queue_update_cond {
    alway_update, only_if_higher, only_if_lower
  };
}

template <typename Key, typename Priority, typename Compare>
class mutable_priority_queue {

    struct kp_pair {
        Key key;
        Priority priority;

        kp_pair (const Key& key, Priority&& priority) :
          key (key), priority (std::forward<Priority> (priority)) {}
    };
    struct kp_pair_compare {
        Compare comp;
        bool operator () (const kp_pair& l, const kp_pair& r) const {
          return comp (l.priority, r.priority);
        }
    };

    using pq_t = boost::heap::pairing_heap<kp_pair, boost::heap::compare<kp_pair_compare>>;
    using handle_t = pq_t::handle_type;

    pq_t heap;
    std::vector<handle_t> key_to_handle;
    const handle_t null_handle;

  public:
    using update_cond = enum ::detail::mutable_priority_queue_update_cond;
    using enum update_cond;

    mutable_priority_queue (size_t max_size) {
      key_to_handle.resize (max_size);
    }

    void clear () {
      size_t s = key_to_handle.size ();
      key_to_handle.clear ();
      heap.clear ();
      key_to_handle.resize (s);
    }

    bool empty () const { return heap.empty (); }
    size_t size () const { return heap.size (); }

    const kp_pair& top () const { return heap.top (); }

    void pop () {
      key_to_handle[heap.top ().key] = null_handle;
      heap.pop ();
    }

    void remove (const Key& key) {
      auto h = key_to_handle[key];
      heap.erase (h);
      key_to_handle[key] = null_handle;
    }

    /** Sets the priority for the given key. If not present, it will be added,
     *  otherwise it will be updated. */
    bool set (const Key& key, Priority&& priority, update_cond update_if = alway_update) {
      assert (static_cast<size_t> (key) < key_to_handle.size ());
      auto& h = key_to_handle[key];
      if (h == null_handle) {
        h = heap.emplace (key, std::forward<Priority> (priority));
        return true;
      }
      else {
        if ((update_if == alway_update) or
            (update_if == only_if_lower and comp (priority, (*h).priority)) or
            (update_if == only_if_higher and comp ((*h).priority, priority))) {
          (*h).priority = std::forward<Priority> (priority);
          if (update_if == alway_update)
            heap.update (h);
          else if (update_if == only_if_higher)
            heap.increase (h);
          else
            heap.decrease (h);
          return true;
        }
      }
      return false;
    }

    const Priority& get_priority (const Key& key) {
      return (*key_to_handle[key]).priority;
    }
  private:
    Compare comp {};
};
