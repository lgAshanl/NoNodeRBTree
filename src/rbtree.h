#pragma once

#include "stdint.h"
#include "nonoderbtree.h"

namespace RBTree
{
    //////////////////////////////////////////////////////////////////

    struct FakeLock
    {
        inline void lock() { };
        inline void unlock() { };
    };

    //////////////////////////////////////////////////////////////////

    template<class K, class V, class Lock = FakeLock>
    class RBTree
    {
        struct Node
        {
            template<typename... Args>
            Node(K&& key, Args&&... args)
              : m_parent(nullptr),
                m_left(nullptr),
                m_right(nullptr),
                m_key(std::forward<K>(key)),
                m_value(std::forward<Args>(args)...)
            { }

            template<typename... Args>
            Node(const K& key, Args&&... args)
              : Node(K{key}, std::forward<Args>(args)...)
            { }

            Node() = delete;

            Node* m_parent;
            Node* m_left;
            Node* m_right;
            K m_key;
            V m_value;
        };

    public:

        class iterator;

        RBTree()
          : m_tree()
        { }

        ~RBTree()
        { clear(); }

        RBTree(const RBTree& other) = delete;
        RBTree(RBTree&& other) noexcept = delete;
        RBTree& operator=(const RBTree& other) = delete;
        RBTree& operator=(RBTree&& other) noexcept = delete;

        template<typename... Args>
        std::pair<iterator, bool> emplace(const K& key, Args&&... args);

        template<typename... Args>
        std::pair<iterator, bool> emplace(K&& key, Args&&... args);

        std::pair<iterator, bool> insert(K const key, V const value);

        std::pair<iterator, bool> insert(const std::pair<K, V>& value);

        size_t erase(K key);

        void clear() noexcept;

        size_t size() const noexcept;

    public:

        class iterator : public std::iterator<std::input_iterator_tag, V> {
            friend class RBTree<K, V, Lock>;

            iterator(typename NoNodeRBTree<K, Node*>::iterator it) : m_it(it) { }

        public:

            iterator(const iterator& it) : m_it(it.m_it) { }
            ~iterator() = default;

            iterator& operator=(const iterator& it) { m_it = it.m_it; return *this; }

            const V& operator*() const noexcept { return m_it->m_value; }
            std::pair<K, V> operator*() { return std::pair<K, V>(m_it->m_key, m_it->m_value); }
            V operator->() const { return m_it; }

            iterator& operator++() { ++m_it; return *this; }
            iterator operator++(int) { iterator it(*this); ++m_it; return it; }

            bool operator==(const iterator& other) const { return m_it == other.m_it; }
            bool operator!=(const iterator& other) const { return m_it != other.m_it; }

        private:

            typename NoNodeRBTree<K, Node*>::iterator m_it;
        };

        iterator begin() const { return iterator(m_tree.begin()); }
        iterator end()   const { return iterator(m_tree.end());   }

    public:

        bool checkRB() { return m_tree.checkRB(); }

    private:

        NoNodeRBTree<K, Node*> m_tree;

    private:

        Lock m_lock;
    };

    //--------------------------------------------------------------//
    template<class K, class V, class L>
    template<typename... Args>
    std::pair<typename RBTree<K, V, L>::iterator, bool> RBTree<K, V, L>::emplace(const K& key, Args&&... args)
    {
        RBTree<K, V, L>::Node* node = new Node(key, std::forward<Args>(args)...);

        // no guard
        // for simple remove of fake lock by optimizer
        m_lock.lock();

        const auto res = m_tree.insert(node);

        m_lock.unlock();

        return std::pair<iterator, bool>(iterator(res.first), res.second);
    }

    //--------------------------------------------------------------//
    template<class K, class V, class L>
    template<typename... Args>
    std::pair<typename RBTree<K, V, L>::iterator, bool> RBTree<K, V, L>::emplace(K&& key, Args&&... args)
    {
        RBTree<K, V, L>::Node* node =
            new Node(std::forward<K>(key), std::forward<Args>(args)...);

        // no guard
        // for simple remove of fake lock by optimizer
        m_lock.lock();

        const auto res = m_tree.insert(node);

        m_lock.unlock();

        return std::pair<iterator, bool>(iterator(res.first), res.second);
    }

    //--------------------------------------------------------------//
    template<class K, class V, class L>
    std::pair<typename RBTree<K, V, L>::iterator, bool> RBTree<K, V, L>::insert(K const key, V const value)
    {
        RBTree<K, V, L>::Node* node = new Node(key, value);

        // no guard
        // for simple remove of fake lock by optimizer
        m_lock.lock();

        const auto res = m_tree.insert(node);

        m_lock.unlock();

        return std::pair<iterator, bool>(iterator(res.first), res.second);
    }

    //--------------------------------------------------------------//
    template<class K, class V, class L>
    std::pair<typename RBTree<K, V, L>::iterator, bool> RBTree<K, V, L>::insert(const std::pair<K, V>& value)
    {
        RBTree<K, V, L>::Node* node = new Node(std::pair<K, V>(value));

        // no guard
        // for simple remove of fake lock by optimizer
        m_lock.lock();

        const auto res = m_tree.insert(node);

        m_lock.unlock();

        return std::pair<iterator, bool>(iterator(res.first), res.second);
    }

    //--------------------------------------------------------------//
    template<class K, class V, class L>
    size_t RBTree<K, V, L>::erase(K key)
    {
        // no guard
        // for simple remove of fake lock by optimizer
        m_lock.lock();

        const auto iter = m_tree.find(key);

        if (m_tree.end() == iter)
        {
            m_lock.unlock();

            return 0;
        }

        const auto res = m_tree.erase(key);
        m_lock.unlock();

        Node* const node = *iter;
        delete node;

        assert(1 == res);

        return 1;
    }

    //--------------------------------------------------------------//
    template<class K, class V, class L>
    void RBTree<K, V, L>::clear() noexcept
    {
        m_tree.clearWithDestruct();
    }

    //--------------------------------------------------------------//
    template<class K, class V, class L>
    size_t RBTree<K, V, L>::size() const noexcept
    {
        return m_tree.size();
    }

    //--------------------------------------------------------------//

}
