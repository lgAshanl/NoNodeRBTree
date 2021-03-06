 # NoNodeRBTree

 ## Два КЧ-дерева: NoNodeRBTree<K,V> и RBTree<K, V, Lock>

 # NoNodeRBTree<K,V>
 * Key (K) - любой, т.ч. Compare не кидает исключений
 * Value (V) - указатель на класс, содержащий публичные поля m_left, m_right, m_parent, m_key для использования деревом.
 * Такая реализация позволяет избежать аллокации служебной ноды при вставке и т.п.
   * Можно гарантировать отсутствие исключений (особенно полезно при использовании под блокировкой, т.к. исчезают лишние синхронизации в крит. секции)
   * Нет, собственно, вызова аллокатора. Это так же чрезвычайно полезно при использовании дерева под блокировкой

 # RBTree<K, V, Lock>
 * Key (K), Value (V) - любой
 * Lock - любой класс лока (с методами lock(), unlock()). Стандартный - пустой лок.
 * Фасад для NoNodeRBTree<K,V>. Позволяет использовать дерево с любыми классами, при этом используя все преимущества NoNodeRBTree<K,V> для блокировки.
