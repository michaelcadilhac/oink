#pragma once

#include <boost/pool/object_pool.hpp>
#include <functional>

/// An allocator that immediately delegates to new/delete.
template <typename Data>
struct new_delete_allocator {
    new_delete_allocator (size_t) {}
    static Data* construct () { return new Data (); }
    static void  destroy (Data* d) { delete (d); }
};

/// The object allocator from Boost.
template <typename Data>
using boost_allocator = boost::object_pool<Data>;

/// A pooling allocator that does not free but keeps tabs of what has been
/// destroyed.
template <typename T>
class recycling_allocator {
  public:
    recycling_allocator (size_t init) : stashes { stash_t (init) }, current_stash { stashes.back () } {}
    ~recycling_allocator () { assert (used == 0); }

    /// Construct a new element.
    /// @return A pointer to a fresh element.
    T* construct () {
      T* ret;
      ++used;

      if (last_used < current_stash.get ().size ())
        ret = &(current_stash.get ()[last_used++]);
      else
        if (not free_Ts.empty ()) {
          auto s = free_Ts.top ();
          free_Ts.pop ();
          ret = s;
        }
        else {
          stashes.emplace_back (2 * current_stash.get ().size ());
          current_stash = std::ref (stashes.back ());
          last_used = 0;
          ret = &(current_stash.get ()[last_used++]);
        }
      return ret;
    }

    /// Releases an element.
    /// @arg t An id as returned by \a construct
    void destroy (T* t) {
      if (t == &(current_stash.get ()[last_used - 1]))
        --last_used;
      else
        free_Ts.push (t);
      --used;
    }

    /// Indicates whether no element is currently allocated.
    bool empty () {
      return used == 0;
    }

  private:
    using stash_t = std::vector<T>;
    std::vector<stash_t> stashes;
    std::reference_wrapper<stash_t> current_stash;
    std::stack<T*> free_Ts;
    size_t last_used;
    size_t used;
};
