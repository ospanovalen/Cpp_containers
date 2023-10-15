#include <iostream>
#include <type_traits>
#include <vector>

template <typename T, typename Allocator = std::allocator<T>>
class Deque {
 public:
  Deque() = default;

  explicit Deque(const Allocator& alloc) : alloc_(alloc) {}

  Deque(const Deque& other) {
    alloc_ =
        allocator_traits::select_on_container_copy_construction(other.alloc_);
    auto begin = other.begin();
    auto end = other.end();
    for (auto iter = begin; iter != end; ++iter) {
      try {
        emplace_back(*iter);
      } catch (...) {
        while (size_ != 0) {
          pop_back();
        }
        dealloc();
        throw;
      }
    }
  }

  explicit Deque(size_t count, const Allocator& alloc = Allocator()) {
    alloc_ = alloc;
    for (size_t it = 0; it < count; ++it) {
      try {
        emplace_back();
      } catch (...) {
        while (size_ != 0) {
          pop_back();
        }
        dealloc();
        throw;
      }
    }
  }

  Deque(size_t count, const T& value, const Allocator& alloc = Allocator()) {
    alloc_ = alloc;
    for (size_t it = 0; it < count; ++it) {
      try {
        emplace_back(value);
      } catch (...) {
        while (size_ != 0) {
          pop_back();
        }
        dealloc();
        throw;
      }
    }
  }

  Deque(Deque&& other) noexcept {
    body_ = other.body_;
    first_in_array_index_ = other.first_in_array_index_;
    last_in_array_index_ = other.last_in_array_index_;
    first_array_index_ = other.first_array_index_;
    last_array_index_ = other.last_array_index_;
    size_ = other.size_;
    capacity_of_arr_ = other.capacity_of_arr_;
    alloc_ = other.alloc_;
    other.body_.clear();
    reset(other.first_in_array_index_);
    reset(other.last_in_array_index_);
    reset(other.first_array_index_);
    reset(other.last_array_index_);
    reset(other.size_);
    reset(other.capacity_of_arr_);
  }

  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator()) {
    alloc_ = alloc;
    for (const auto& iter : init) {
      try {
        emplace_back(iter);
      } catch (...) {
        while (size_ != 0) {
          pop_back();
        }
        dealloc();
        throw;
      }
    }
  }

  ~Deque() {
    while (size_ != 0) {
      pop_back();
    }
    dealloc();
  }

  Deque& operator=(const Deque& other) {
    if (this == &other) {
      return *this;
    }
    Deque tmp(other);
    if constexpr (allocator_traits::propagate_on_container_copy_assignment::
                      value) {
      alloc_ = other.alloc_;
    }
    while (size() != 0) {
      pop_back();
    }
    dealloc();
    std::swap(body_, tmp.body_);
    std::swap(first_in_array_index_, tmp.first_in_array_index_);
    std::swap(first_array_index_, tmp.first_array_index_);
    std::swap(last_in_array_index_, tmp.last_in_array_index_);
    std::swap(last_array_index_, tmp.last_array_index_);
    std::swap(size_, tmp.size_);
    std::swap(capacity_of_arr_, tmp.capacity_of_arr_);
    return *this;
  }

  Deque& operator=(Deque&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    while (size_ != 0) {
      pop_back();
    }
    dealloc();
    body_ = std::move(other.body_);
    first_in_array_index_ = other.first_in_array_index_;
    last_in_array_index_ = other.last_in_array_index_;
    first_array_index_ = other.first_array_index_;
    last_array_index_ = other.last_array_index_;
    size_ = other.size_;
    capacity_of_arr_ = other.capacity_of_arr_;
    alloc_ = std::move(other.alloc_);
    other.body_.clear();
    reset(other.first_in_array_index_);
    reset(other.last_in_array_index_);
    reset(other.first_array_index_);
    reset(other.last_array_index_);
    reset(other.size_);
    reset(other.capacity_of_arr_);
    return *this;
  }

  size_t size() { return size_; }

  [[nodiscard]] size_t size() const { return size_; }

  [[maybe_unused]] bool empty() { return size() == 0; }

  [[nodiscard]] Allocator get_allocator() const { return alloc_; }

  T& at(const size_t& index) {
    check_index(index);
    return body_[(index + first_in_array_index_) / kArraySize +
                 first_array_index_]
                [(index + first_in_array_index_) % kArraySize];
  }

  [[nodiscard]] const T& at(const size_t& index) const {
    check_index(index);
    return body_[(index + first_in_array_index_) / kArraySize +
                 first_array_index_]
                [(index + first_in_array_index_) % kArraySize];
  }

  T& operator[](const size_t& index) {
    return body_[(index + first_in_array_index_) / kArraySize +
                 first_array_index_]
                [(index + first_in_array_index_) % kArraySize];
  }

  const T& operator[](const size_t& index) const {
    return body_[(index + first_in_array_index_) / kArraySize +
                 first_array_index_]
                [(index + first_in_array_index_) % kArraySize];
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    if (capacity() != 0) {
      if (back_increase_capacity_condition()) {
        increase_capacity();
      }
      if (last_in_array_index_ + 1 == kArraySize) {
        if (last_array_index_ + 1 == body_.size()) {
          increase_capacity();
        }
        last_array_index_++;
        last_in_array_index_ = 0;
      } else {
        last_in_array_index_++;
      }
      try {
        array_traits::construct(alloc_, last_element(),
                                std::forward<Args>(args)...);
      } catch (...) {
        if (last_in_array_index_ == 0) {
          last_array_index_--;
          last_in_array_index_ = kArraySize - 1;
        } else {
          last_in_array_index_--;
        }
        throw;
      }
      ++size_;
    } else {
      initial_allocate();
      try {
        array_traits::construct(alloc_, last_element(),
                                std::forward<Args>(args)...);
      } catch (...) {
        dealloc();
        throw;
      }
      ++size_;
    }
  }

  void push_back(const T& value) { emplace_back(value); }

  void push_back(T&& value) { emplace_back(std::move(value)); }

  void decrease_capacity() {
    auto prev_array_size =
        last_array_index_ * kArraySize + last_in_array_index_ + 1;
    if (prev_array_size <= size_ / 4) {
      auto new_capacity = size_ / 2 + 1;
      if (new_capacity >= capacity_of_arr_ / 2) {
        new_capacity = capacity_of_arr_ / 2;
      }
      if (new_capacity >= kArraySize) {
        std::vector<T*> new_body(new_capacity, nullptr);
        for (size_t i = 0; i < new_capacity; ++i) {
          new_body[i] = array_traits::allocate(alloc_, kArraySize);
        }
        size_t new_first_array_index = (new_capacity - size_) / 2;
        size_t new_last_array_index = new_first_array_index + size_ - 1;
        size_t new_first_in_array_index = kArraySize - size_ % kArraySize;
        size_t new_last_in_array_index = kArraySize - 1;
        for (size_t i = new_first_array_index, j = first_array_index_;
             i <= new_last_array_index; ++i, ++j) {
          for (size_t k = 0; k < kArraySize; ++k) {
            array_traits::construct(alloc_, &new_body[i][k],
                                    std::move(body_[j][k]));
            array_traits::destroy(alloc_, &body_[j][k]);
          }
          array_traits::deallocate(alloc_, body_[j], kArraySize);
        }
        body_.swap(new_body);
        capacity_of_arr_ = new_capacity;
        first_array_index_ = new_first_array_index;
        last_array_index_ = new_last_array_index;
        first_in_array_index_ = new_first_in_array_index;
        last_in_array_index_ = new_last_in_array_index;
      }
    }
  }

  void pop_back() {
    if (empty()) {
      throw std::runtime_error("Try to pop from an empty deque");
    }

    array_traits::destroy(alloc_,
                          &body_[last_array_index_][last_in_array_index_]);
    if (first_in_array_index_ != last_in_array_index_ ||
        first_array_index_ != last_array_index_) {
      if (last_in_array_index_ == 0) {
        last_in_array_index_ = kArraySize - 1;
        last_array_index_ -= 1;
      } else {
        --last_in_array_index_;
      }
    } else {
      if (last_array_index_ > 0) {
        decrease_capacity();
      }
    }

    --size_;
  }

  template <typename... Args>
  void emplace_front(Args&&... args) {
    if (capacity() != 0) {
      if (front_increase_capacity_condition()) {
        increase_capacity();
      }
      if (first_in_array_index_ == 0) {
        if (first_array_index_ == 0) {
          increase_capacity();
        }
        first_array_index_--;
        first_in_array_index_ = kArraySize - 1;
      } else {
        first_in_array_index_--;
      }

      try {
        array_traits::construct(alloc_, first_element(),
                                std::forward<Args>(args)...);
      } catch (...) {
        if (first_in_array_index_ == kArraySize - 1) {
          first_array_index_++;
          first_in_array_index_ = 0;
        } else {
          first_in_array_index_++;
        }
        throw;
      }
      ++size_;
    } else {
      initial_allocate();
      try {
        array_traits::construct(alloc_, first_element(),
                                std::forward<Args>(args)...);
      } catch (...) {
        dealloc();
        throw;
      }
      ++size_;
    }
  }

  void push_front(const T& value) { emplace_front(value); }

  void push_front(T&& value) { emplace_front(std::move(value)); }

  void pop_front() {
    if (empty()) {
      throw std::runtime_error("Try to pop from an empty deque");
    }

    array_traits::destroy(alloc_,
                          &body_[first_array_index_][first_in_array_index_]);
    if (first_in_array_index_ != last_in_array_index_ ||
        first_array_index_ != last_array_index_) {
      if (first_in_array_index_ == kArraySize - 1) {
        first_in_array_index_ = 0;
        first_array_index_ += 1;
      } else {
        ++first_in_array_index_;
      }
    } else {
      if (first_array_index_ > 0) {
        decrease_capacity();
      }
    }

    --size_;
  }

  template <bool IsConst>
  class Iterator {
   public:
    using value_type = std::conditional_t<IsConst, const T, T>;
    using pointer = value_type*;
    using iterator_category = std::random_access_iterator_tag;
    using reference = value_type&;
    using diff = std::ptrdiff_t;

    Iterator() : body_(nullptr), index_of_array_(0), index_in_array_(0) {}

    Iterator(size_t index_of_array, size_t index_in_array,
             const std::vector<T*>* body)
        : body_(body),
          index_of_array_(index_of_array),
          index_in_array_(index_in_array) {}

    Iterator<IsConst>& operator++() {
      if (index_in_array_ == kArraySize - 1) {
        index_in_array_ = 0;
        ++index_of_array_;
      } else {
        ++index_in_array_;
      }
      return *this;
    }

    Iterator<IsConst> operator++(int) {
      Iterator<IsConst> copy(*this);
      ++(*this);
      return copy;
    }

    Iterator<IsConst>& operator--() {
      if (index_in_array_ > 0) {
        --index_in_array_;
      } else {
        --index_of_array_;
        index_in_array_ = kArraySize - 1;
      }
      return *this;
    }

    Iterator<IsConst> operator--(int) {
      Iterator<IsConst> copy(*this);
      --(*this);
      return copy;
    }

    Iterator<IsConst>& operator+=(diff n) {
      if (n >= 0) {
        size_t total_index = index_of_array_ * kArraySize + index_in_array_ + n;
        index_of_array_ = total_index / kArraySize;
        index_in_array_ = total_index % kArraySize;
      } else {
        diff neg_n = -n;
        if (index_in_array_ >= static_cast<size_t>(neg_n)) {
          index_in_array_ -= static_cast<size_t>(neg_n);
        } else {
          diff remaining = neg_n - index_in_array_ - 1;
          diff num_arrays_to_skip = remaining / kArraySize + 1;
          index_of_array_ -= num_arrays_to_skip;
          index_in_array_ = kArraySize - 1 - remaining % kArraySize;
        }
      }
      return *this;
    }

    Iterator<IsConst> operator+(diff n) const {
      Iterator<IsConst> copy(*this);
      copy += n;
      return copy;
    }

    Iterator<IsConst>& operator-=(diff n) { return (*this) += (-n); }

    Iterator<IsConst> operator-(diff n) const {
      Iterator<IsConst> copy(*this);
      copy -= n;
      return copy;
    }

    bool operator<(const Iterator<IsConst>& other) const {
      return other > (*this);
    }

    bool operator>(const Iterator<IsConst>& other) const {
      return (*this - other) > 0;
    }

    bool operator<=(const Iterator<IsConst>& other) const {
      return !(*this > other);
    }

    bool operator>=(const Iterator<IsConst>& other) const {
      return !(*this < other);
    }

    bool operator==(const Iterator<IsConst>& other) const {
      return body_ == other.body_ && index_of_array_ == other.index_of_array_ &&
             index_in_array_ == other.index_in_array_;
    }

    bool operator!=(const Iterator<IsConst>& other) const {
      return !(*this == other);
    }

    diff operator-(const Iterator<IsConst>& other) const {
      return static_cast<diff>(index_of_array_) * kArraySize + index_in_array_ -
             static_cast<diff>(other.index_of_array_) * kArraySize -
             other.index_in_array_;
    }

    reference operator*() const {
      return (*body_)[index_of_array_][index_in_array_];
    }

    reference operator[](diff idx) const { return *(*this + idx); }

    pointer operator->() const {
      return &((*body_)[index_of_array_][index_in_array_]);
    }

   private:
    const std::vector<T*>* body_;
    size_t index_of_array_;
    size_t index_in_array_;
    static const size_t kArraySize = 512;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  [[nodiscard]] iterator begin() const {
    if (size() != 0) {
      return iterator(first_array_index_, first_in_array_index_, &body_);
    }
    if (last_in_array_index_ + 1 != kArraySize) {
      return iterator(last_array_index_, last_in_array_index_ + 1, &body_);
    }
    return iterator(last_array_index_ + 1, 0, &body_);
  }

  [[nodiscard]] const_iterator cbegin() const {
    if (size() != 0) {
      return const_iterator(first_array_index_, first_in_array_index_, &body_);
    }
    if (last_in_array_index_ != kArraySize - 1) {
      return const_iterator(last_array_index_, last_in_array_index_ + 1,
                            &body_);
    }
    return const_iterator(last_array_index_ + 1, 0, &body_);
  }

  [[nodiscard]] iterator end() const {
    if (last_in_array_index_ != kArraySize - 1) {
      return iterator(last_array_index_, last_in_array_index_ + 1, &body_);
    }
    return iterator(last_array_index_ + 1, 0, &body_);
  }

  [[nodiscard]] const_iterator cend() const {
    if (last_in_array_index_ != kArraySize - 1) {
      return const_iterator(last_array_index_, last_in_array_index_ + 1,
                            &body_);
    }
    return const_iterator(last_array_index_ + 1, 0, &body_);
  }

  [[nodiscard]] reverse_iterator rbegin() const {
    return reverse_iterator(end());
  }

  [[nodiscard]] reverse_iterator rend() const {
    return reverse_iterator(begin());
  }

  [[nodiscard]] const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }

  [[nodiscard]] const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  iterator insert(iterator iter, const T& value) {
    if (iter == begin()) {
      emplace_front(value);
      return begin();
    }
    if (iter == end()) {
      emplace_back(value);
      return end() - 1;
    }
    emplace_front(value);
    for (auto cycle_iter = begin(); cycle_iter <= iter; ++cycle_iter) {
      std::swap(*cycle_iter, *(cycle_iter + 1));
    }
    return iter - 1;
  }

  template <typename... Args>
  iterator emplace(const_iterator c_iterator, Args&&... args) {
    return insert(c_iterator, std::forward<Args>(args)...);
  }

  iterator erase(iterator iter) {
    if (iter == begin()) {
      pop_front();
      return begin();
    }
    if (iter == end() - 1) {
      pop_back();
      return end();
    }
    for (auto cycle_iter = iter; cycle_iter > begin(); --cycle_iter) {
      *(cycle_iter) = *(cycle_iter - 1);
    }
    pop_front();
    return iter;
  }

 private:
  size_t capacity() { return capacity_of_arr_; }

  auto last_element() {
    return &body_[last_array_index_][last_in_array_index_];
  }

  auto first_element() {
    return &body_[first_array_index_][first_in_array_index_];
  }

  bool back_increase_capacity_condition() {
    return (capacity_of_arr_ == last_array_index_ + 1 &&
            kArraySize == last_in_array_index_ + 2);
  }

  bool front_increase_capacity_condition() {
    return (first_array_index_ == 0 && first_in_array_index_ == 0);
  }

  void check_index(size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("Index out of range");
    }
  }

  void set_indexes_in_array() {
    first_in_array_index_ = kArraySize / 2;
    last_in_array_index_ = kArraySize / 2;
  }

  void set_array_indexes() {
    first_array_index_ = capacity_of_arr_ / 2;
    last_array_index_ = capacity_of_arr_ / 2;
  }

  void increase_capacity() {
    const size_t kNextCapacity = capacity_of_arr_ * 3;
    std::vector<T*> tmp(kNextCapacity);
    for (size_t i = 0; i < capacity_of_arr_; ++i) {
      tmp[i] = body_[i];
    }
    for (size_t i = capacity_of_arr_; i < kNextCapacity; ++i) {
      tmp[i] = array_traits::allocate(alloc_, kArraySize);
    }
    first_array_index_ += capacity_of_arr_;
    last_array_index_ += capacity_of_arr_;
    capacity_of_arr_ = kNextCapacity;

    std::swap(tmp, body_);
  }

  void initial_allocate() {
    capacity_of_arr_ = kStartCapacityOfArr;
    body_.resize(capacity_of_arr_);
    for (auto& iterator : body_) {
      iterator = array_traits::allocate(alloc_, kArraySize);
    }
    set_indexes_in_array();
    set_array_indexes();
  }

  void dealloc() {
    for (auto& iterator : body_) {
      array_traits::deallocate(alloc_, iterator, kArraySize);
    }
    reset(capacity_of_arr_);
    reset(size_);
    reset(first_array_index_);
    reset(last_array_index_);
    reset(first_in_array_index_);
    reset(last_in_array_index_);
    body_.resize(0);
  }

  template <typename K>
  void reset(K& member) {
    member = 0;
  }

  std::vector<T*> body_;
  size_t size_ = 0;
  size_t capacity_of_arr_ = 0;
  const size_t kStartCapacityOfArr = 64;

  using allocator_traits = std::allocator_traits<Allocator>;
  using allocator = typename allocator_traits::template rebind_alloc<T>;
  using array_traits = std::allocator_traits<allocator>;
  allocator alloc_;
  size_t first_in_array_index_ = 0;
  size_t last_in_array_index_ = 0;
  const size_t kArraySize = 512;
  size_t first_array_index_ = 0;
  size_t last_array_index_ = 0;
};
