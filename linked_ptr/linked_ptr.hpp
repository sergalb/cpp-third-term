//
// Created by Sergey on 25.02.2019.
//

#ifndef LINKED_PTR_LINKEDPTR_H
#define LINKED_PTR_LINKEDPTR_H


#include <type_traits>

namespace smart_ptr {
    namespace details {
        class node {
        public:
            node *getLeft() const {
                return left;
            }

            node *getRight() const {
                return right;
            }


            ~node() {
                if (left != nullptr) {
                    left->right = right;
                }
                if (right != nullptr) {
                    right->left = left;
                }
                left = nullptr;
                right = nullptr;
            }

            node(node &other) {
                if (other.right != nullptr) {
                    right->left = this;
                }
                right = other.right;
                left = &other;
                other.right = this;
            }

            node &operator=(node const &other) {
                if (other.right != nullptr) {
                    right->left = this;
                }
                right = other.right;
                left = const_cast<node *>(&other);
                other.right = this;
            }

            node() : left(nullptr), right(nullptr) {};

            bool is_last() {
                return left == nullptr && right == nullptr;
            }

            void swap(node &other) noexcept {
                if (left != nullptr) {
                    left->right = &other;
                }
                if (right != nullptr) {
                    right->left = &other;
                }
                if (other.left != nullptr) {
                    other.left->right = this;
                }
                if (other.right != nullptr) {
                    other.right->left = this;
                }
                left = other.left;
                right = other.right;
                other.left = left;
                other.right = right;
            }

        private:
            mutable node *left;
            mutable node *right;
        };

        void swap(node &first, node &second) {
            first.swap(second);
        }
    }

    template<typename T>
    class linked_ptr {
    public:
        constexpr linked_ptr() noexcept = default;

        explicit linked_ptr(T *data) noexcept : data(data) {};

        template<typename U, typename = std::enable_if<std::is_convertible_v<U *, T *>>>
        explicit linked_ptr(U *data) noexcept : data(data) {};

        linked_ptr(linked_ptr<T> const &other) noexcept : data(other.data), _node(other._node) {};

        template<typename U, typename = std::enable_if<std::is_convertible_v<U *, T *>>>
        explicit linked_ptr(linked_ptr<U> const &other) noexcept : data(data), _node(other._node) {};


        linked_ptr<T> &operator=(linked_ptr<T> const &other) noexcept {
            _node = other._node;
            data = other.data;
            return *this;
        }
        ~linked_ptr() noexcept {
            if (_node.is_last() && data != nullptr) {
                delete data;
            }
            data = nullptr;
            _node.~node();

        }

        template<typename U, typename = std::enable_if<std::is_convertible_v<U *, T *>>>
        linked_ptr<T> &operator=(linked_ptr<U> const &other) noexcept {
            _node = details::node(other._node);
            data = other.data;
        }


        void reset(T *new_data = nullptr) {
            _node.~node();
            data = new_data;
        }

        template<typename U, typename = std::enable_if<std::is_convertible_v<U *, T *>>>
        void reset(U *new_data) {
            _node.~node();
            data = new_data;
        }

        void swap(linked_ptr<T> &other) noexcept {
            std::swap(data, other.data);
            _node.swap(other._node);
        }

        T *get() const noexcept {
            return data;
        }

        bool unique() const noexcept {
            return _node.is_last();
        }

        T &operator*() const noexcept {
            return *get();
        }

        T *operator->() const noexcept {
            return get();
        }

        operator bool() const noexcept {
            return data;
        }


    public:
        T *data;
        mutable details::node _node;
    };

    template<typename T, typename U>
    bool operator==(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first.get() == second.get();
    }

    template<typename T, typename U>
    bool operator!=(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first.get() != second.get();
    }


    template<typename T, typename U>
    bool operator<(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first.get() < second.get();
    }

    template<typename T, typename U>
    bool operator>(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return second < first;
    }
    template<typename T, typename U>
    bool operator<=(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first == second || first < second;
    }

    template<typename T, typename U>
    bool operator>=(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first == second || first > second;
    }

    template <typename T>
    linked_ptr<T> make_linked_ptr(T* data) {
        return linked_ptr<T>(data);
    }
}
#endif //LINKED_PTR_LINKEDPTR_H
