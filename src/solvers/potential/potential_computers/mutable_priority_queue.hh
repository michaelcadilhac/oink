#pragma once

// Boost has mutable PQ, but they perform worse than this straightforward
// implementation due to:
//
//   https://github.com/Ten0/updatable_priority_queue (LGPL-3.0)
//


#include <vector>

namespace detail {
  enum class mutable_priority_queue_update_cond {
    alway_update, only_if_higher, only_if_lower
  };
}

template <typename Key, typename Priority, typename Compare>
class mutable_priority_queue {
    struct priority_queue_node {
        Priority priority;
        Key key;
        priority_queue_node (const Key& key, Priority&& priority) :
          priority (std::forward<Priority> (priority)), key (key) {}
    };

  private:
    std::vector<size_t> id_to_heappos;
    std::vector<priority_queue_node> heap;
    Compare comp;

  public:
    using update_cond = enum ::detail::mutable_priority_queue_update_cond;
    using enum update_cond;

    mutable_priority_queue (size_t init_reserve) {
      id_to_heappos.reserve (init_reserve);
      heap.reserve (init_reserve);
    }

    bool empty () const { return heap.empty (); }
    size_t size () const { return heap.size (); }

    const priority_queue_node& top () const { return heap.front (); }

    void pop () {
      if (size () == 0) return;

      id_to_heappos[heap.front ().key] = SIZE_MAX;

      if (size () > 1) {
        *heap.begin () = std::move (*(heap.end () - 1));
        id_to_heappos[heap.front ().key] = 0;
      }

      heap.pop_back ();
      sift_down (0);
    }

    /** Sets the priority for the given key. If not present, it will be added,
     *  otherwise it will be updated. */
    void set (const Key& key, Priority&& priority, update_cond update_if = alway_update) {
      if (static_cast<size_t> (key) < id_to_heappos.size () and
          id_to_heappos[key] != SIZE_MAX) // This key is already in the pQ
        update (key, std::move (priority), update_if);
      else
        push (key, std::move (priority));
    }

    std::pair<bool, const Priority&> get_priority (const Key& key) {
      if (key < id_to_heappos.size ()) {
        size_t pos = id_to_heappos[key];

        if (pos < ((size_t) -2)) {
          return {true, heap[pos].priority};
        }
      }

      return {false, 0};
    }

    void push (const Key& key, Priority&& priority) {
      if (id_to_heappos.size () < static_cast<size_t> (key) + 1)
        id_to_heappos.resize (key + 1, SIZE_MAX);
      assert (id_to_heappos[key] == SIZE_MAX);
      size_t n = heap.size ();
      id_to_heappos[key] = n;
      heap.emplace_back (key, std::forward<Priority> (priority));
      sift_up (n);
    }

    void update (const Key& key, Priority&& new_priority, update_cond update_if) {
      assert (static_cast<size_t> (key) < id_to_heappos.size ());
      size_t heappos = id_to_heappos[key];
      assert (heappos != SIZE_MAX);

      Priority& priority = heap[heappos].priority;

      if (update_if != only_if_lower and comp (priority, new_priority)) {
        priority = std::forward<Priority> (new_priority);
        sift_up (heappos);
      } else if (update_if != only_if_higher and comp (new_priority, priority)) {
        priority = std::forward<Priority> (new_priority);
        sift_down (heappos);
      }
    }

    void clear () {
      // These leave capacity unchanged.
      id_to_heappos.clear ();
      heap.clear ();
    }

  private:
    void sift_down (size_t heappos) {
      size_t len = heap.size ();
      size_t child = heappos * 2 + 1;

      if (len < 2 or child >= len) return;

      // max of children:
      if (child + 1 < len and comp (heap[child].priority, heap[child + 1].priority))
        ++child;

      if (not (comp (heap[heappos].priority, heap[child].priority)))
        return; // Already in heap order

      priority_queue_node val = std::move (heap[heappos]); // extract it

      do {
        heap[heappos] = std::move (heap[child]);
        id_to_heappos[heap[heappos].key] = heappos;
        heappos = child;
        child = 2 * child + 1;

        if (child >= len) break;

        if (child + 1 < len and comp (heap[child].priority, heap[child + 1].priority))
          ++child;
      } while (comp (val.priority, heap[child].priority));

      heap[heappos] = std::move (val);
      id_to_heappos[heap[heappos].key] = heappos;
    }

    void sift_up (size_t heappos) {
      size_t len = heap.size ();

      if (len < 2 or heappos <= 0) return;

      size_t parent = (heappos - 1) / 2;

      if (not (comp (heap[parent].priority, heap[heappos].priority))) return;

      priority_queue_node val = std::move (heap[heappos]);

      do {
        heap[heappos] = std::move (heap[parent]);
        id_to_heappos[heap[heappos].key] = heappos;
        heappos = parent;

        if (heappos <= 0) break;

        parent = (parent - 1) / 2;
      } while (comp (heap[parent].priority, val.priority));

      heap[heappos] = std::move (val);
      id_to_heappos[heap[heappos].key] = heappos;
    }
};
