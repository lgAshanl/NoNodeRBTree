#include "stdint.h"
#include <queue>

namespace RBTree
{

    //////////////////////////////////////////////////////////////////

    template<class K, class V>
    class NoNodeRBTree
    {
        // ptr: 0bXXXXX...XXXY
        // Y - color (0 - black, 1 - red)

    public:

        class iterator;

        NoNodeRBTree()
          : m_root(nullptr), m_size(0)
        { }

        NoNodeRBTree(const NoNodeRBTree& other) = delete;
        NoNodeRBTree(NoNodeRBTree&& other) noexcept = delete;
        NoNodeRBTree& operator=(const NoNodeRBTree& other) = delete;
        NoNodeRBTree& operator=(NoNodeRBTree&& other) noexcept = delete;

        iterator find(K key) noexcept;

        std::pair<iterator, bool> insert(V value) noexcept;

        // TODO: by iterator
        size_t erase(K key) noexcept;

        void clear() noexcept;

        void clearWithDestruct() noexcept;

        size_t size() const noexcept;

    private:

        void repairInsert(V parent, V node) noexcept;

        void repairErase(V parent, V brother) noexcept;

    public:

        class iterator  : public std::iterator<std::input_iterator_tag, V> {
            friend class NoNodeRBTree<K, V>;

            iterator(V node) : m_node(node) { }

        public:

            iterator(const iterator& it) : m_node(it.m_node) { }
            ~iterator() = default;

            iterator& operator=(const iterator& it) { m_node = it.m_node; return *this; }

            const V& operator*() const noexcept { return m_node; }
            std::pair<K, V> operator*() { return std::pair<K, V>(m_node->m_key, m_node); }
            V operator->() const { return m_node; }

            iterator& operator++() { m_node = next(m_node); return *this; }
            iterator operator++(int) { iterator it(*this); ++(*this); return it; }

            bool operator==(const iterator& other) const { return m_node == other.m_node; }
            bool operator!=(const iterator& other) const { return m_node != other.m_node; }

        private:

            V m_node;
        };

        iterator begin() const
        {
            if (nullptr == m_root)
                return end();
            return iterator(maxLeft(m_root));
        }

        iterator end() const
        {
            return iterator(nullptr);
        }

    public:

        bool checkRB();

    private:

        static V next(V node);

        static inline V maxLeft(V node);

        static void erase_swap(V one, V other);

    private:

        static V uncle(V const parent);

        static inline void pred_rotate(V const parent, V const node, V const grandpa);

        static inline void rotate_left(V const parent, V const node);

        static inline void rotate_right(V const parent, V const node);

        static inline bool isChildsBlack(V node);

    private:

        static inline size_t color(V node);

        static inline bool is_node_black(V node);

        static inline bool is_node_red(V node);

        static inline void set_parent_save_color(V node, V parent);

        static inline V red(V node);

        static inline V black(V ptr);

        static inline V pure(V ptr);

        static inline void assert_pure(V ptr);

    private:

        V m_root;

        size_t m_size;
    };

    //--------------------------------------------------------------//

    template<class K, class V>
    typename NoNodeRBTree<K, V>::iterator NoNodeRBTree<K, V>::find(K key) noexcept
    {
        V node = m_root;
        while (key != node->m_key)
        {
            V const next = pure((key > node->m_key) ? node->m_right : node->m_left);

            if (nullptr == next)
                return end();
            else
                node = next;
        }

        return iterator(node);
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    std::pair<typename NoNodeRBTree<K, V>::iterator, bool> NoNodeRBTree<K, V>::insert(V const value) noexcept
    {
        const K key = value->m_key;

        if (nullptr == m_root)
        {
            m_root = value;
            value->m_key = key;
            value->m_left = nullptr;
            value->m_right = nullptr;
            value->m_parent = nullptr;
            ++m_size;
            return std::pair<iterator, bool>(iterator(value), true);
        }
        else
        {
            V node = m_root;
            while (true)
            {
                if (key == node->m_key)
                    return std::pair<iterator, bool>(iterator(node), false);

                V const next = pure((key > node->m_key) ? node->m_right : node->m_left);

                if (nullptr == next)
                        break;
                else
                    node = next;
            }

            if (key > node->m_key)
                node->m_right = value;
            else
                node->m_left = value;

            value->m_parent = red(node);
            value->m_key = key;
            value->m_left = nullptr;
            value->m_right = nullptr;
            ++m_size;

            if (is_node_black(node))
            {
                return std::pair<iterator, bool>(iterator(value), true);
            }

            repairInsert(node, value);

            return std::pair<iterator, bool>(iterator(value), true);
        }
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    size_t NoNodeRBTree<K, V>::erase(K key) noexcept
    {
        if (nullptr == m_root)
        {
            return 0;
        }
        else
        {
            V node = m_root;
            while (key != node->m_key)
            {
                V const next = pure((key > node->m_key) ? node->m_right : node->m_left);

                if (nullptr == next)
                    return 0;
                else
                    node = next;
            }

            assert(node->m_key == key);

            if ((nullptr != node->m_left) && (nullptr != node->m_right))
            {
                V min_right = maxLeft(pure(node->m_right));
                if (nullptr == node->m_parent)
                    m_root = min_right;

                erase_swap(node, min_right);

            }

            const V parent = pure(node->m_parent);
            if (is_node_red(node))
            {
                assert((nullptr == node->m_left) && (nullptr == node->m_right));
                if (node == parent->m_left)
                    parent->m_left = nullptr;
                else
                    parent->m_right = nullptr;

                --m_size;

                return 1;
            }

            V child = (nullptr != node->m_left) ? node->m_left : node->m_right;

            if (nullptr != child) //if (is_node_red(child))
            {
                assert(is_node_red(child));
                if (nullptr != parent)
                {
                    if (node == parent->m_left)
                        parent->m_left = child;
                    else
                        parent->m_right = child;
                }
                else
                {
                    m_root = child;
                }
                child->m_parent = parent; // black

                --m_size;

                return 1;
            }

            assert((nullptr == node->m_left) && (nullptr == node->m_right));
            assert(nullptr == child);

            // case 1
            if (nullptr == parent) //(m_root == node)
            {
                m_root = nullptr;
                --m_size;
                return 1;
            }

            if (node == parent->m_left)
                parent->m_left = child;
            else
                parent->m_right = child;

            // repair

            V brother = (nullptr == parent->m_left) ? parent->m_right : parent->m_left;
            repairErase(parent, brother);

            --m_size;

            return 1;
        }
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::clear() noexcept
    {
        m_root = nullptr;
        m_size = 0;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::clearWithDestruct() noexcept
    {
        V node = m_root;
        while (nullptr != node)
        {
            V next = node->m_left;
            if (nullptr == next)
            {
                next = node->m_right;
                if (nullptr == next)
                {
                    next = pure(node->m_parent);
                    if (nullptr != next)
                    {
                        if (node == next->m_left)
                            next->m_left = nullptr;
                        else
                            next->m_right = nullptr;
                    }
                    delete node;
                }
            }

            node = next;
        }
        
        m_root = nullptr;
        m_size = 0;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    size_t NoNodeRBTree<K, V>::size() const noexcept
    {
        return m_size;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::repairInsert(V parent, V node) noexcept
    {
        // grandfather definitely exists and is black (parent is red)
        V grandpa = pure(parent->m_parent);
        V uncle = pure((pure(grandpa->m_left) == parent) ? grandpa->m_right : grandpa->m_left);
        //while (is_ptr_red(uncle))
        while ((nullptr != uncle) && is_node_red(uncle))
        {
            parent->m_parent = grandpa; // black
            uncle->m_parent = grandpa; // black

            if (nullptr == grandpa->m_parent)
            {
                return;
            }

            V const grandgrandpa = pure(grandpa->m_parent);
            grandpa->m_parent = red(grandgrandpa);

            if (is_node_black(grandgrandpa))
            {
                return;
            }

            parent = grandgrandpa;
            node = grandpa;
            grandpa = pure(grandgrandpa->m_parent);
            uncle = pure((pure(grandpa->m_left) == parent) ? grandpa->m_right : grandpa->m_left);
        }

        // uncle is black
        if (pure(parent->m_right) == node && pure(grandpa->m_left) == parent)
        {
            pred_rotate(parent, node, grandpa);
            rotate_left(parent, node);
            node->m_parent = red(node->m_parent);
            //parent->m_parent = red(parent->m_parent);
            std::swap(parent, node);
        }
        else if (pure(parent->m_left) == node && pure(grandpa->m_right) == parent)
        {
            pred_rotate(parent, node, grandpa);
            rotate_right(parent, node);
            node->m_parent = red(node->m_parent);
            //parent->m_parent = red(parent->m_parent);
            std::swap(parent, node);
        }

        // case 5 (cascade)
        //pred_rotate(grandpa, parent, grandgrandpa);
        {
            assert(is_node_black(grandpa));
            V const grandgrandpa = grandpa->m_parent; // pure (black)
            parent->m_parent = grandgrandpa;
            if (nullptr != grandgrandpa)
            {
                if (pure(grandgrandpa->m_left) == grandpa)
                    grandgrandpa->m_left = parent;
                else
                    grandgrandpa->m_right = parent;
            }
            else
            {
                if (m_root == grandpa)
                    m_root = parent;
            }
        }
        if (pure(parent->m_left) == node) // && pure(grandpa->m_left) == parent)
        {
            rotate_right(grandpa, parent);
        }
        else // pure(parent->m_right) == node) && pure(grandpa->m_right) == parent)
        {
            rotate_left(grandpa, parent);
        }
    }

    template<class K, class V>
    void NoNodeRBTree<K, V>::repairErase(V parent, V brother) noexcept
    {
        assert(nullptr != parent);
        assert(nullptr != brother);
        assert_pure(parent);
        assert_pure(brother);

        // case 2
        if (is_node_red(brother))
        {
            assert(is_node_black(parent));

            V grandpa = pure(parent->m_parent);
            brother->m_parent = grandpa;
            if (nullptr != grandpa)
            {
                if (pure(grandpa->m_left) == parent)
                    grandpa->m_left = brother;
                else
                    grandpa->m_right = brother;
            }
            else
            {
                m_root = brother;
            }

            if (brother == parent->m_right)
            {
                rotate_left(parent, brother);
                assert(is_node_black(brother));
                brother = parent->m_right;
            }
            else
            {
                rotate_right(parent, brother);
                assert(is_node_black(brother));
                brother = parent->m_left;
            }

            // post
            assert(is_node_red(parent));
        }
        assert(is_node_black(brother));

        // case 3
        const bool is_childs_black = isChildsBlack(brother);
        if (is_node_black(parent) && is_childs_black)
        {
            brother->m_parent = red(brother->m_parent);

            const V grandpa = pure(parent->m_parent);
            if (nullptr == grandpa)
            {
                return;
            }

            brother = (parent == grandpa->m_left) ? grandpa->m_right : grandpa->m_left;

            return repairErase(grandpa, brother);
        }

        // case 4
        if (is_node_red(parent) && is_childs_black)
        {
            brother->m_parent = red(brother->m_parent);
            parent->m_parent = black(parent->m_parent);
            return;
        }

        const size_t old_parent_color = color(parent);
        V left_brother_child = pure(brother->m_left);
        V right_brother_child = pure(brother->m_right);
        if (brother == parent->m_right)
        {
            // case 5
            if (nullptr == right_brother_child || is_node_black(right_brother_child))
            {
                assert(is_node_red(brother->m_left));

                pred_rotate(brother, left_brother_child, parent);

                rotate_right(brother, left_brother_child);

                brother = left_brother_child;
            }
            assert(is_node_black(brother));
            assert(is_node_red(brother->m_right));

            // case 6
            {
                const V grandpa = pure(parent->m_parent);
                brother->m_parent = (V)((size_t)grandpa | old_parent_color);
                if (nullptr != grandpa)
                {
                    if (pure(grandpa->m_left) == parent)
                        grandpa->m_left = brother;
                    else
                        grandpa->m_right = brother;
                }
                else
                {
                    m_root = brother;
                }
            }
            
            parent->m_right = brother->m_left;
            if (nullptr != brother->m_left)
                set_parent_save_color(brother->m_left, parent);

            parent->m_parent = brother; // black
            brother->m_left = parent;
            brother->m_right->m_parent = black(brother->m_right->m_parent);
        }
        else
        {
            V right_brother_child = pure(brother->m_right);
            // case 5
            if (nullptr == left_brother_child || is_node_black(left_brother_child))
            {
                assert(is_node_red(brother->m_right));

                pred_rotate(brother, right_brother_child, parent);

                rotate_left(brother, right_brother_child);

                brother = right_brother_child;
            }
            assert(is_node_black(brother));
            assert(is_node_red(brother->m_left));

            // case 6
            {
                const V grandpa = pure(parent->m_parent);
                brother->m_parent = (V)((size_t)grandpa | old_parent_color);
                if (nullptr != grandpa)
                {
                    if (pure(grandpa->m_left) == parent)
                        grandpa->m_left = brother;
                    else
                        grandpa->m_right = brother;
                }
                else
                {
                    m_root = brother;
                }
            }
            
            parent->m_left = brother->m_right;
            if (nullptr != brother->m_right)
                set_parent_save_color(brother->m_right, parent);

            parent->m_parent = brother; // black
            brother->m_right = parent;
            brother->m_left->m_parent = black(brother->m_left->m_parent);
        }   
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    bool NoNodeRBTree<K, V>::checkRB()
    {
        if (nullptr == m_root)
        {
            assert(0 == m_size);
            return true;
        }
        assert(is_node_black(m_root));

        size_t size = 1;

        uint32_t depth = 0;

        std::queue<std::pair<V, uint32_t>> queue;

        queue.emplace(m_root->m_left, 1);
        queue.emplace(m_root->m_right, 1);

        while(queue.size())
        {
            std::pair<V, uint32_t> p = queue.front();
            queue.pop();
            V node = p.first;
            uint32_t d = p.second;

            if (nullptr == node)
            {
                if (0 == depth)
                    depth = d;

                assert(depth == d);
            }
            else
            {
                assert(nullptr != node->m_parent);
                ++size;
                const bool is_black = is_node_black(node);
                const V parent = pure(node->m_parent);
                
                assert(!is_node_red(parent) || is_black);

                if (is_black)
                    ++d;

                queue.emplace(node->m_left, d);
                queue.emplace(node->m_right, d);
            }
        }
        assert(m_size == size);

        return true;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    V NoNodeRBTree<K, V>::next(V node)
    {
        if (nullptr != node->m_right)
        {
            return maxLeft(pure(node->m_right));
        }

        V parent = pure(node->m_parent);
        while (nullptr != parent && parent->m_key <= node->m_key)
        {
            parent = pure(parent->m_parent);
        }
        
        return parent;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    V NoNodeRBTree<K, V>::maxLeft(V node)
    {
        while (nullptr != node->m_left)
            node = pure(node->m_left);

        return node;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::erase_swap(V one, V other)
    {
        // one may be root
        assert(nullptr != other->m_parent);
        assert(one->m_key < other->m_key); // other is maxLeft right son of one
        assert(nullptr == other->m_left);

        V parent_one = pure(one->m_parent);
        V parent_other = pure(other->m_parent);

        if (nullptr != parent_one)
        {
            if (one == parent_one->m_left)
                parent_one->m_left = other;
            else
                parent_one->m_right = other;
        }

        if (nullptr != other->m_right)
            set_parent_save_color(other->m_right, one);

        if (nullptr != one->m_left)
            set_parent_save_color(one->m_left, other);

        const V old_other_right = other->m_right;
        //const size_t old_other_color = color(other->m_parent);
        V new_parent_one;

        if (one == parent_other)
        {
            new_parent_one = (V)((size_t)other | color(other));

            other->m_right = one;
        }
        else
        {
            new_parent_one = other->m_parent;

            parent_other->m_left = one;

            if (nullptr != one->m_right)
                set_parent_save_color(one->m_right, other);

            other->m_right = one->m_right;
        }

        other->m_left = one->m_left;
        other->m_parent = one->m_parent;

        one->m_left = nullptr;
        one->m_right = old_other_right;
        one->m_parent = new_parent_one;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    V NoNodeRBTree<K, V>::uncle(V const parent)
    {
        assert_pure(parent);

        V const grandpa = pure(parent->m_parent);
        assert(nullptr != grandpa);

        if (parent == pure(grandpa->m_left))
            return grandpa->m_right;
        else
            return grandpa->m_left;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::pred_rotate(V const parent, V const node, V const grandpa)
    {
        assert_pure(parent);
        assert_pure(node);
        assert_pure(grandpa);
        assert(nullptr != node);
        assert(nullptr != parent);

        node->m_parent = grandpa; // red?
        if (pure(grandpa->m_left) == parent)
            grandpa->m_left = node;
        else
            grandpa->m_right = node;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::rotate_left(V const parent, V const node)
    {
        parent->m_right = node->m_left;
        if (nullptr != node->m_left)
            set_parent_save_color(node->m_left, parent);

        parent->m_parent = red(node);
        node->m_left = parent;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::rotate_right(V const parent, V const node)
    {        
        parent->m_left = node->m_right;
        if (nullptr != node->m_right)
            set_parent_save_color(node->m_right, parent);

        parent->m_parent = red(node);
        node->m_right = parent;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    bool NoNodeRBTree<K, V>::isChildsBlack(V node)
    {
        return ((nullptr == node->m_left)  || is_node_black(node->m_left)) &&
            ((nullptr == node->m_right) || is_node_black(node->m_right));
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    size_t NoNodeRBTree<K, V>::color(V node)
    {
        return (size_t)node->m_parent & (size_t)1;
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    bool NoNodeRBTree<K, V>::is_node_black(V node)
    {
        return 0 == ((size_t)node->m_parent & (size_t)1);
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    bool NoNodeRBTree<K, V>::is_node_red(V node)
    {
        return 0 != ((size_t)node->m_parent & (size_t)1);
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::set_parent_save_color(V node, V parent)
    {
        assert_pure(parent);
        node->m_parent = (V)((size_t)parent | color(node));
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    V NoNodeRBTree<K, V>::red(V node)
    {
        return (V)((size_t)node | (size_t)1);
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    V NoNodeRBTree<K, V>::black(V ptr)
    {
        return (V)((size_t)ptr & (~((size_t)0b1)));
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    V NoNodeRBTree<K, V>::pure(V ptr)
    {
        return (V)((size_t)ptr & (~((size_t)0b111)));
    }

    //--------------------------------------------------------------//

    template<class K, class V>
    void NoNodeRBTree<K, V>::assert_pure(V ptr)
    {
        assert(0 == (((size_t)ptr) & (size_t)0b111));
    }

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
            Node(K key, V value)
              : m_parent(nullptr),
                m_left(nullptr),
                m_right(nullptr),
                m_key(key),
                m_value(value)
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

        RBTree(const RBTree& other) = delete;
        RBTree(RBTree&& other) noexcept = delete;
        RBTree& operator=(const RBTree& other) = delete;
        RBTree& operator=(RBTree&& other) noexcept = delete;

        std::pair<iterator, bool> insert(K const key, V const value);

        size_t erase(K key);

        // TODO:
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
