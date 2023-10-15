#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* prev = this;
    BaseNode* next = this;
  };

  struct Node : BaseNode {
    T value;
    Node(){};
    Node(const T& val) : value(val) {}
  };
  BaseNode fake_;
  size_t size_ = 0;
  using alloc = std::allocator<T>;
  using node_alloc =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using node_traits = typename std::allocator_traits<node_alloc>;
  node_alloc alloc_;
  void push_back_balance(Node* new_node) {
    new_node->prev = fake_.prev;
    new_node->next = &fake_;
    fake_.prev->next = new_node;
    fake_.prev = new_node;
  }
  void pop_back_balance(Node* node) {
    node->prev->next = &fake_;
    fake_.prev = node->prev;
  }
  void push_front_balance(Node* new_node) {
    new_node->prev = &fake_;
    new_node->next = fake_.next;
    fake_.next->prev = new_node;
    fake_.next = new_node;
  }
  void pop_front_balance(Node* node) {
    node->next->prev = &fake_;
    fake_.next = node->next;
  }
  void push_back_from_default_value() {
    Node* new_node = node_traits::allocate(alloc_, 1);
    try {
      node_traits::construct(alloc_, new_node);
    } catch (...) {
      node_traits::deallocate(alloc_, new_node, 1);
      throw;
    }
    push_back_balance(new_node);
    ++size_;
  }
  void swap(List& other) noexcept {
    while (size_ != 0) {
      pop_back();
    }
    std::swap(fake_, other.fake_);
    std::swap(size_, other.size_);
  }

 public:
  using value_type = T;
  using allocator_type = Allocator;
  template <bool IsConst>
  class MyIterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::conditional_t<IsConst, const T, T>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    MyIterator(BaseNode* node) : head_(node) {}
    reference operator*() { return static_cast<Node*>(head_)->value; }
    pointer operator->() const { return &static_cast<Node*>(head_)->value; }
    MyIterator& operator++() {
      head_ = head_->next;
      return *this;
    }
    MyIterator operator++(int) {
      MyIterator old = *this;
      ++(*this);
      return old;
    }
    MyIterator& operator--() {
      head_ = head_->prev;
      return *this;
    }
    MyIterator operator--(int) {
      MyIterator old = *this;
      --(*this);
      return old;
    }
    bool operator==(const MyIterator& other) const {
      return head_ == other.head_;
    }

    bool operator!=(const MyIterator& other) const {
      return head_ != other.head_;
    }

   private:
    BaseNode* head_;
  };
  using iterator = MyIterator<false>;
  using const_iterator = MyIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() const {
    return iterator(static_cast<BaseNode*>(fake_.next));
  }
  iterator end() const { return iterator(const_cast<BaseNode*>(&fake_)); }
  const_iterator cbegin() const { return const_iterator(fake_.next); }
  const_iterator cend() const { return const_iterator(&fake_); }
  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }
  List() = default;
  explicit List(size_t count, const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    for (size_t i = 0; i < count; ++i) {
      try {
        push_back_from_default_value();
      } catch (...) {
        while (!empty()) {
          pop_back();
        }
      }
    }
  }
  List(size_t count, const T& value, const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    for (size_t i = 0; i < count; ++i) {
      try {
        push_back(value);
      } catch (...) {
        while (!empty()) {
          pop_back();
        }
        throw;
      }
    }
  }
  List(std::initializer_list<T> init, const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    for (const auto& value : init) {
      try {
        push_back(value);
      } catch (...) {
        while (!empty()) {
          pop_back();
        }
        throw;
      }
    }
  }
  List(const List& other)
      : size_(0),
        alloc_(
            node_traits::select_on_container_copy_construction(other.alloc_)) {
    for (const auto& value : other) {
      try {
        push_back(value);
      } catch (...) {
        while (!empty()) {
          pop_back();
        }
        throw;
      }
    }
  }
  List& operator=(const List& other) {
    List temp(other);
    swap(temp);
    if (node_traits::propagate_on_container_copy_assignment::value &&
        alloc_ != other.alloc_) {
      alloc_ = other.alloc_;
    }
    return *this;
  }

  ~List() {
    while (!empty()) {
      pop_back();
    }
  }
  void push_back(const T& value) {
    Node* new_node = node_traits::allocate(alloc_, 1);
    try {
      node_traits::construct(alloc_, new_node, value);
    } catch (...) {
      node_traits::deallocate(alloc_, new_node, 1);
      throw;
    }
    push_back_balance(new_node);
    ++size_;
  }
  void push_front(const T& value) {
    Node* new_node = node_traits::allocate(alloc_, 1);
    try {
      node_traits::construct(alloc_, new_node, value);
    } catch (...) {
      node_traits::deallocate(alloc_, new_node, 1);
      throw;
    }
    push_front_balance(new_node);
    ++size_;
  }
  void pop_back() {
    Node* last_node = reinterpret_cast<Node*>(fake_.prev);
    pop_back_balance(last_node);
    node_traits ::destroy(alloc_, last_node);
    node_traits::deallocate(alloc_, last_node, 1);
    --size_;
  }
  void pop_front() {
    Node* first_node = reinterpret_cast<Node*>(fake_.next);
    pop_front_balance(first_node);
    node_traits::destroy(alloc_, first_node);
    node_traits::deallocate(alloc_, first_node, 1);
    --size_;
  }
  size_t size() const { return size_; }
  bool empty() const { return size() == 0; }
  node_alloc get_allocator() const { return alloc_; }
};
