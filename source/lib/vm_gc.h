#pragma once
#include <cassert>
#include <string>
#include <set>
#include <vector>

#include "value.h"

namespace ccml {

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
// ultra simple halt-the-world garbage collector
//
struct value_gc_t {

  // todo: create a memory pool of values to use

  value_t *new_int(int64_t value);

  value_t *new_array(int64_t value);

  value_t *new_string(const std::string &value);

  value_t *new_none();

  value_t *copy(const value_t &v);

  void collect(value_t **input, size_t count);

  ~value_gc_t() {
    for (value_t *v : commit_) {
      delete_(v);
    }
    commit_.clear();
  }

protected:

  void release_(value_t *v) {
    assert(v);
    if (v->array_) {
      // todo: pool arrays?
      assert(v->type == val_type_array);
      delete[] v->array_;
      v->array_ = nullptr;
    }
    avail_.push_back(v);
  }

  void delete_(value_t *v) {
    assert(v);
    if (v->array_) {
      assert(v->type == val_type_array);
      delete[] v->array_;
    }
    delete v;
  }

  value_t *alloc_() {
    value_t *v = nullptr;
    if (avail_.empty()) {
      // allocate a new value
      v = new value_t;
      assert(v);
      commit_.push_back(v);
    }
    else {
      // pop from front
      v = avail_.back();
      assert(v);
      avail_.pop_back();
    }
    allocs_.insert(v);
    return v;
  }

  void visit_(std::set<const value_t*> &s, const value_t *v);

  // known dead values
  std::vector<value_t*> avail_;
  // values that have been given out and may be alive
  std::set<value_t*> allocs_;
  // all values that have been allocated with new
  std::vector<value_t*> commit_;
};

} // namespce ccml
