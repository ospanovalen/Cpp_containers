#include <iostream>
#include <memory>

/// <h1> Interface Declaration
template <std::totally_ordered T>
class SearchTree {
 public:
  virtual void insert(const T&) = 0;
  virtual void insert(T&&) = 0;

  virtual void remove() = 0;
  virtual void remove(T) = 0;

  virtual T& min() = 0;
  virtual T& max() = 0;

  virtual bool hasElement() const = 0;
};

/// <h1> BinaryTree Declaration
template <std::totally_ordered T>
class BinaryTree : public SearchTree<T> {
 public:
  BinaryTree() = default;
  BinaryTree(const BinaryTree&);
  BinaryTree(BinaryTree&&);

  void insert(const T&) override;
  void insert(T&&) override;

  template<typename ...Args>
  void emplace(Args&&... args);

  void remove() override;
  void remove(T) override;

  T& min() override;
  T& max() override;

  bool hasElement(T&) const override;

  BinaryTree getLeftSubtree();
  BinaryTree getRightSubtree();

  T top();

 private:
  class Node;
  using NodePtr = std::shared_ptr<Node>;
  using NodeWeakPtr = std::weak_ptr<Node>;

  NodePtr getMaxNode(NodePtr);
  NodePtr getMinNode(NodePtr);

  BinaryTree(const NodePtr&);

  NodePtr root_{nullptr};
};

/// <h1> Node Declaration
template <std::totally_ordered T>
class BinaryTree<T>::Node :
        public std::enable_shared_from_this<BinaryTree<T>::Node> {
 public:
  /// <b> base constructors
  Node(const T&);
  Node(T&&);

  /// <b> constructor for any value
  template <typename ...Args>
  Node(Args&&...);

  /// <b> get right child
  NodePtr getLeft() const;

  /// <b> get left child
  NodePtr getRight() const;

  /// <b> creates shared_ptr on node's parent
  NodePtr getRoot() const;

  /// <b> returns value, contained by this node
  T& getValue();
  const T& getValue() const;

  /// <b> set left/right child
  void setLeft(const NodePtr&);
  void setRight(const NodePtr&);

  /// <b> swap node with 'other' (only pointers)
  void swap(Node& other);

  /// <b> create copy of node and of it's subtree
  NodePtr getCopy() const;

  /// <b> true if node don't have children
  bool isLeaf() const;

  /// <b> destroy this node
  void reset();

 private:
  using Base = std::enable_shared_from_this<Node>;

  T value_;
  bool is_left_child_{false};

  NodePtr left_{nullptr};
  NodePtr right_{nullptr};
  NodeWeakPtr parent_;
};

/// <h1> Node Implementation </h1>
template <std::totally_ordered T>
BinaryTree<T>::Node::Node(const T& value) : value_(value) {}

template <std::totally_ordered T>
BinaryTree<T>::Node::Node(T&& value) : value_(std::move(value)) {}

template <std::totally_ordered T>
template <typename ...Args>
BinaryTree<T>::Node::Node(Args&&... args)
    : value_(std::forward<Args>(args)...) {}

template <std::totally_ordered T>
typename BinaryTree<T>::NodePtr BinaryTree<T>::Node::getLeft() const {
  return left_;
}

template <std::totally_ordered T>
typename BinaryTree<T>::NodePtr BinaryTree<T>::Node::getRight() const {
  return right_;
}

template <std::totally_ordered T>
typename BinaryTree<T>::NodePtr BinaryTree<T>::Node::getRoot() const {
  return parent_.lock();
}

template <std::totally_ordered T>
T& BinaryTree<T>::Node::getValue() { return value_; }

template <std::totally_ordered T>
const T& BinaryTree<T>::Node::getValue() const { return value_; }

template <std::totally_ordered T>
void BinaryTree<T>::Node::setLeft(const NodePtr& node) {
  left_ = node;
  node->parent_ = std::weak_ptr(Base::shared_from_this());
  node->is_left_child_ = true;
}

template <std::totally_ordered T>
void BinaryTree<T>::Node::setRight(const NodePtr& node) {
  right_ = node;
  node->parent_ = std::weak_ptr(Base::shared_from_this());
  node->is_left_child_ = false;
}

template <std::totally_ordered T>
void BinaryTree<T>::Node::swap(BinaryTree<T>::Node& other) {
  std::swap(value_, other.value_);
}

template <std::totally_ordered T>
typename BinaryTree<T>::NodePtr BinaryTree<T>::Node::getCopy() const {
  NodePtr node = std::make_shared(value_);
  if (left_ != nullptr) {
    node.left_ = left_.getCopy();
    node.left_->parent_ = std::weak_ptr(Base::shared_from_this());
  }
  if (right_ != nullptr) {
    node.right_ = right_.getCopy();
    node.right_->parent_ = std::weak_ptr(Base::shared_from_this());
  }
  return node
}

template <std::totally_ordered T>
bool BinaryTree<T>::Node::isLeaf() const {
  return getLeft() == nullptr && getRight() == nullptr;
}

template <std::totally_ordered T>
void BinaryTree<T>::Node::reset() {
  NodePtr parent = getParent();
  if (is_left_child_) {
    parent->getLeft().reset();
  } else {
    parent->getRight().reset();
  }
}

/// <h1> BinaryTree Implementation

template <std::totally_ordered T>
BinaryTree<T>::BinaryTree(const BinaryTree& other) : root_(other.root_->getCopy()) {}

template <std::totally_ordered T>
BinaryTree<T>::BinaryTree(BinaryTree<T>&& other) : root_(std::move(other.root_)) {}

template <std::totally_ordered T>
void BinaryTree<T>::insert(const T& value) { emplace(value); }

template <std::totally_ordered T>
void BinaryTree<T>::insert(T&& value) { emplace(std::move(value)); }

template <std::totally_ordered T>
template <typename ...Args>
void BinaryTree<T>::emplace(Args&&... args) {
  NodePtr node = std::make_shared<Node>(std::forward<Args>(args)...);
  if (hasElement(node->getValue())) {
    return;
  }
  NodePtr current = root_;
  while (!current.isLeaf()) {
    if (current->getValue() < node->getValue()) {
      if (current->getRight() == nullptr) {
        current->setRight(*node);
        return;
      }
      current = current->getRight();
    }
    if (current->getLeft() == nullptr) {
      current->setLeft(*node);
      return;
    }
    current = current->getLeft();
  }
  if (current->getValue() < node->getValue()) {
    current->setRight(*node);
  } else {
    current->setLeft(*node);
  }
}

template <std::totally_ordered T>
void BinaryTree<T>::remove() {
  if (root_ == nullptr) {
    return;
  }
  NodePtr current = root_;
  while (!current->isLeaf()) {
    if (current->getLeft() != nullptr) {
      current = getMaxNode(current->getLeft());
      continue;
    }
    if (current->getRight() != nullptr) {
      current = getMaxNode(current->getRight());
      continue;
    }
  }
  current->reset();
}

template <std::totally_ordered T>
void BinaryTree<T>::remove(T value) {
  if (!hasElement(value)) {
    return;
  }
  NodePtr node_to_delete = root_;
  while (!node_to_delete->getValue() != value) {
    if (node_to_delete->getValue() < value) {
      node_to_delete = node_to_delete->getRight();
      continue;
    }
    node_to_delete = node_to_delete->getLeft();
  }
  NodePtr current = node_to_delete;
  while (!current->isLeaf()) {
    if (current->getLeft() != nullptr) {
      current = getMaxNode(current->getLeft());
      continue;
    }
    if (current->getRight() != nullptr) {
      current = getMaxNode(current->getRight());
      continue;
    }
  }
  current->reset();
}

template <std::totally_ordered T>
T& BinaryTree<T>::min() {
  return getMinNode(root_).get();
}

template <std::totally_ordered T>
T& BinaryTree<T>::max() {
  return getMaxNode(root_).get();
}

template <std::totally_ordered T>
bool BinaryTree<T>::hasElement(T& value) const {
  if (root_ == nullptr) {
    return false;
  }
  NodePtr current = root_;
  while (current->getValue() != value) {
    if (current->getValue() < value) {
      if (current->getRight() == nullptr) {
        return false;
      }
      current = current->getRight();
      continue;
    }
    if (current->getLeft() == nullptr) {
      return false;
    }
    current = current->getLeft();
  }
  return true;
}

template <std::totally_ordered T>
T BinaryTree<T>::top() {
  return root_->getValue();
}

template <std::totally_ordered T>
BinaryTree<T> BinaryTree<T>::getLeftSubtree() {
  return root_->getleft()->getCopy();
}

template <std::totally_ordered T>
BinaryTree<T> BinaryTree<T>::getRightSubtree() {
  return root_->getRight()->getCopy();
}

template <std::totally_ordered T>
BinaryTree<T>::BinaryTree(const BinaryTree::NodePtr& node) : root_(node) {}

template <std::totally_ordered T>
NodePtr BinaryTree<T>::getMinNode(BinaryTree::NodePtr node) {
  while (node->getLeft() != nullptr) {
    node = node->getLeft();
  }
  return node;
}

template <std::totally_ordered T>
NodePtr BinaryTree<T>::getMaxNode(BinaryTree::NodePtr node) {
  while (node->getRight() != nullptr) {
    node = node->getRight();
  }
  return node;
}
