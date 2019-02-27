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
            }

            node(node &other) {
                other.right = right;
                other.left = this;
                if (right != nullptr) {
                    right->left = &other;
                }
                right = &other;
            }

            node &operator=(node const &other) {
                other.right = right;
                other.left = this;
                if (right != nullptr) {
                    right->left = const_cast<node *>(&other);
                }
                right = const_cast<node *>(&other);
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

        void swap(node &first, node &second) noexcept {
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
            _node = node(other._node);
            data = other.data;
        }

        template<typename U, typename = std::enable_if<std::is_convertible_v<U *, T *>>>
        linked_ptr<T> &operator=(linked_ptr<U> const &other) noexcept {
            _node = details::node(other._node);
            data = other.data;
        }


        void reset(T *new_data = nullptr) {
            ~_node();
            if (unique()) {
                delete data;
            }
            data = new_data;
        }

        template<typename U, typename = std::enable_if<std::is_convertible_v<U *, T *>>>
        void reset(U *new_data) {
            ~_node();
            if (unique()) {
                delete data;
            }
            data = new_data;
        }

        void swap(linked_ptr<T> &other) noexcept {
            std::swap(data, other.data);
            std::swap(_node, other._node);
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
            return *get();
        }

        operator bool() const noexcept {
            return data;
        }


    private:
        T *data;
        mutable details::node _node;
    };

    template<typename T>
    void swap(linked_ptr<T> &lhs, linked_ptr<T> &rhs) noexcept {
        lhs.swap(rhs);
    }

    template<typename T, typename U>
    bool operator==(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first.data == second.data;
    }

    template<typename T, typename U>
    bool operator!=(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first.data != second.data;
    }


    template<typename T, typename U>
    bool operator<(linked_ptr<T> const &first, linked_ptr<U> const &second) {
        return first.data < second.data;
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

}
#endif //LINKED_PTR_LINKEDPTR_H
