#include <memory>
#include <iterator>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <algorithm>

template<typename T, size_t NodeMaxSize = 10, typename Allocator = std::allocator<T>>
class unrolled_list {
private:
    struct Node {
        size_t size; // Current number of elements
        Node* next; // Pointer to the next node
        Node* prev; // Pointer to the previous node
        Allocator element_allocator; // Allocator for elements
        T* data; // Array of elements

        // Node constructor
        explicit Node(const Allocator& allocator)
            : size(0), next(nullptr), prev(nullptr), element_allocator(allocator) {
            data = static_cast<T*>(::operator new(NodeMaxSize * sizeof(T)));
        }

        // Destructor
        ~Node() {
            if (size > 0) {
                for (size_t i = 0; i < size; ++i) {
                    std::allocator_traits<Allocator>::destroy(element_allocator, data + i);
                }
            }
            ::operator delete(data);
        }

        void push_back(T&& value) {
            new (data + size) T(std::move(value));
            ++size;
        }

        template<typename... Args>
        void emplace_back(Args&&... args) {
            new (data + size) T(std::forward<Args>(args)...);
            ++size;
        }

        // Insert element at position
        void insert(size_t pos, const T& value) {
            for (size_t i = size; i > pos; --i) {
                data[i] = std::move(data[i - 1]);
            }
            new (data + pos) T(value);
            ++size;
        }

        // Insert with move
        void insert(size_t pos, T&& value) {
            for (size_t i = size; i > pos; --i) {
                data[i] = std::move(data[i - 1]);
            }
            new (data + pos) T(std::move(value));
            ++size;
        }

        // Remove element at position
        void erase(size_t pos) {
            std::allocator_traits<Allocator>::destroy(element_allocator, data + pos);
            for (size_t i = pos; i < size - 1; ++i) {
                data[i] = std::move(data[i + 1]);
            }
            --size;
        }

        // Check if node is full
        bool is_full() const { return size == NodeMaxSize; }
    };

    // Allocator for nodes
    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocatorTraits = std::allocator_traits<NodeAllocator>;

    Node* head;
    Node* tail;
    size_t size_; // Total number of elements
    Allocator allocator; // Element allocator
    NodeAllocator node_allocator; // Node allocator
    
    Node* create_node(Node* prev_node = nullptr, Node* next_node = nullptr) {
        Node* new_node = NodeAllocatorTraits::allocate(node_allocator, 1);
        try {
            NodeAllocatorTraits::construct(node_allocator, new_node, allocator);
        } catch (...) {
            NodeAllocatorTraits::deallocate(node_allocator, new_node, 1);
            throw;
        }
        
        new_node->prev = prev_node;
        new_node->next = next_node;
        
        if (prev_node) prev_node->next = new_node;
        if (next_node) next_node->prev = new_node;
        
        if (!head) head = new_node;
        if (next_node == nullptr) tail = new_node;
        
        return new_node;
    }

    void destroy_node(Node* node) noexcept {
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        
        if (node == head) head = node->next;
        if (node == tail) tail = node->prev;
        
        NodeAllocatorTraits::destroy(node_allocator, node);
        NodeAllocatorTraits::deallocate(node_allocator, node, 1);
    }

    // Iterator template class
    template<bool IsConst>
    class iterator_impl {
    private:
        Node* current_node; // Pointer to current list node
        size_t current_pos; // Current position in node's element array

    public:
       // Types needed for compatibility with standard algorithms
        using iterator_category = std::bidirectional_iterator_tag; // Bidirectional iterator
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;

        iterator_impl(Node* node = nullptr, size_t pos = 0)
            : current_node(node), current_pos(pos) {}

        // Dereference
        reference operator*() const {
            return current_node->data[current_pos];
        }

        // Member access (returns pointer to current element)
        pointer operator->() const {
            return &current_node->data[current_pos];
        }

        // Prefix increment
        iterator_impl& operator++() {
            if (current_pos + 1 < current_node->size) {
                ++current_pos;
            } else {
                current_node = current_node->next;
                current_pos = 0;
            }
            return *this;
        }

        // Postfix increment
        iterator_impl operator++(int) {
            iterator_impl tmp = *this;
            ++(*this);
            return tmp;
        }

        // Prefix/postfix decrement
        iterator_impl& operator--() {
            if (current_pos > 0) {
                --current_pos;
            } else {
                current_node = current_node->prev;
                current_pos = current_node ? current_node->size - 1 : 0;
            }
            return *this;
        }

        iterator_impl operator--(int) {
            iterator_impl tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const iterator_impl& other) const {
            return current_node == other.current_node && 
                   current_pos == other.current_pos;
        }

        bool operator!=(const iterator_impl& other) const {
            return !(*this == other);
        }

        // Methods for iterator state
        Node* get_node() const { return current_node; }
        size_t get_pos() const { return current_pos; }

       // Conversion operator to const iterator (only for non-const iterators)
        operator iterator_impl<true>() const requires (!IsConst) {
            return iterator_impl<true>(current_node, current_pos);
        }
    };

public:
    // Standard type definitions
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using iterator = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors
    unrolled_list() : head(nullptr), tail(nullptr), size_(0), allocator(), node_allocator() {}

    explicit unrolled_list(size_type count, const T& value = T(), const Allocator& alloc = Allocator())
        : unrolled_list(alloc) {
        for (size_type i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    unrolled_list(unrolled_list&& other, const Allocator& alloc)
        : allocator(alloc), node_allocator(alloc) {
        if (alloc == other.get_allocator()) {
            head = other.head;
            tail = other.tail;
            size_ = other.size_;
            other.head = nullptr;
            other.tail = nullptr;
            other.size_ = 0;
        } else {
            for (auto&& item : other) {
                push_back(std::move(item));
            }
            other.clear();
        }
    }

   explicit unrolled_list(const Allocator& alloc) 
    : head(nullptr), tail(nullptr), size_(0), allocator(alloc), node_allocator(alloc) {}

    // Copy constructor
    unrolled_list(const unrolled_list& other)
        : unrolled_list(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.allocator)) {
        for (const auto& item : other) {
            push_back(item);
        }
    }

    // Move constructor
    unrolled_list(unrolled_list&& other) noexcept
        : head(other.head), tail(other.tail), size_(other.size_), 
          allocator(std::move(other.allocator)), node_allocator(std::move(other.node_allocator)) {
        other.head = nullptr;
        other.tail = nullptr;
        other.size_ = 0;
    }

    // Initializer list constructor
    unrolled_list(std::initializer_list<T> init, const Allocator& alloc = Allocator())
        : unrolled_list(alloc) {
        for (const auto& item : init) {
            push_back(item);
        }
    }

    // Range constructor
    template<typename InputIt>
    unrolled_list(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : unrolled_list(alloc) {
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    ~unrolled_list() {
        clear();
    }

    // Assignment operators
    unrolled_list& operator=(const unrolled_list& other) {
        if (this != &other) {
            clear();
            if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
                allocator = other.allocator;
                node_allocator = other.node_allocator;
            }
            for (const auto& item : other) {
                push_back(item);
            }
        }
        return *this;
    }

    unrolled_list& operator=(unrolled_list&& other) noexcept(
        std::allocator_traits<Allocator>::is_always_equal::value) {
        if (this != &other) {
            clear();
            if (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                allocator = std::move(other.allocator);
                node_allocator = std::move(other.node_allocator);
            }
            head = other.head;
            tail = other.tail;
            size_ = other.size_;
            other.head = nullptr;
            other.tail = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    unrolled_list& operator=(std::initializer_list<T> ilist) {
        clear();
        for (const auto& item : ilist) {
            push_back(item);
        }
        return *this;
    }

    allocator_type get_allocator() const noexcept {
        return allocator;
    }

    // Element access
    reference front() {
        return head->data[0];
    }

    const_reference front() const {
        return head->data[0];
    }

    reference back() {
        return tail->data[tail->size - 1];
    }

    const_reference back() const {
        return tail->data[tail->size - 1];
    }

    reference operator[](size_type pos) {
        auto it = begin();
        std::advance(it, pos);
        return *it;
    }

    const_reference operator[](size_type pos) const {
        auto it = begin();
        std::advance(it, pos);
        return *it;
    }

    // Iterators
    iterator begin() noexcept {
        return iterator(head, 0);
    }

    const_iterator begin() const noexcept {
        return const_iterator(head, 0);
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(head, 0);
    }

    iterator end() noexcept {
        return iterator(nullptr, 0);
    }

    const_iterator end() const noexcept {
        return const_iterator(nullptr, 0);
    }

    const_iterator cend() const noexcept {
        return const_iterator(nullptr, 0);
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }


    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    // Size
    bool empty() const noexcept {
        return size_ == 0;
    }

    size_type size() const noexcept {
        return size_;
    }

    size_type max_size() const noexcept {
        return std::allocator_traits<NodeAllocator>::max_size(node_allocator) * NodeMaxSize;
    }

    // Modifiers
    void clear() noexcept {
        while (head) {
            Node* next = head->next;
            destroy_node(head);
            head = next;
        }
        tail = nullptr;
        size_ = 0;
    }

    // Insert operations
    iterator insert(const_iterator pos, size_type count, const T& value) {
        if (count == 0) return iterator(pos.get_node(), pos.get_pos());
        
        iterator result;
        for (size_type i = 0; i < count; ++i) {
            result = insert(pos, value);
        }
        return result;
    }

    iterator insert(const_iterator pos, const T& value) {
        return emplace(pos, value);
    }

    iterator insert(const_iterator pos, T&& value) {
        return emplace(pos, std::move(value));
    }

    iterator insert(iterator pos, const T& value) {
        return insert(const_iterator(pos), value);
    }

    iterator insert(iterator pos, T&& value) {
        return insert(const_iterator(pos), std::move(value));
    }

    // Emplace element in-place
    template<typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        if (pos == end()) {
            emplace_back(std::forward<Args>(args)...);
            return iterator(tail, tail->size - 1);
        }

        Node* node = pos.get_node();
        size_t pos_in_node = pos.get_pos();

        if (!node->is_full()) {
            node->insert(pos_in_node, T(std::forward<Args>(args)...));
            ++size_;
            return iterator(node, pos_in_node);
        }

        Node* new_node = create_node(node, node->next); // Split full node
        size_t half = NodeMaxSize / 2;
        
        for (size_t i = half; i < NodeMaxSize; ++i) { // Move elements to new node
            new_node->emplace_back(std::move(node->data[i]));
            std::allocator_traits<Allocator>::destroy(allocator, node->data + i);
        }
        node->size = half;
        new_node->size = NodeMaxSize - half;

        if (pos_in_node < half) { // Insert into appropriate node
            node->insert(pos_in_node, T(std::forward<Args>(args)...));
            ++size_;
            return iterator(node, pos_in_node);
        } else {
            new_node->insert(pos_in_node - half, T(std::forward<Args>(args)...));
            ++size_;
            return iterator(new_node, pos_in_node - half);
        }
    }

    // Erase operations
    iterator erase(const_iterator pos) noexcept {
        Node* node = pos.get_node();
        size_t pos_in_node = pos.get_pos();

        node->erase(pos_in_node);
        --size_;

        if (node->size == 0 && node != head) {
            Node* next_node = node->next;
            destroy_node(node);
            return iterator(next_node, 0);
        }

        return iterator(node, pos_in_node < node->size ? pos_in_node : 0);
    }

    iterator erase(const_iterator first, const_iterator last) noexcept {
        while (first != last) {
            first = erase(first);
        }
        return iterator(first.get_node(), first.get_pos());
    }

    // Push/pop operations
    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    template<typename... Args>
    reference emplace_back(Args&&... args) {
        if (empty()) {
            create_node();
        }

        if (tail->is_full()) {
            create_node(tail, nullptr);
        }

        tail->emplace_back(std::forward<Args>(args)...);
        ++size_;
        return tail->data[tail->size - 1];
    }

    void pop_back() noexcept {
        if (empty()) return;

        std::allocator_traits<Allocator>::destroy(allocator, tail->data + tail->size - 1);
        --tail->size;
        --size_;

        if (tail->size == 0 && tail != head) {
            Node* prev = tail->prev;
            destroy_node(tail);
            tail = prev;
        }
    }

    void push_front(const T& value) {
        emplace_front(value);
    }

    void push_front(T&& value) {
        emplace_front(std::move(value));
    }

    template<typename... Args>
    reference emplace_front(Args&&... args) {
        if (empty()) {
            create_node();
        }

        if (head->is_full()) {
            Node* new_node = create_node(nullptr, head);
            head = new_node;
        }

        head->insert(0, T(std::forward<Args>(args)...));
        ++size_;
        return head->data[0];
    }

    void pop_front() noexcept {
        if (empty()) return;

        std::allocator_traits<Allocator>::destroy(allocator, head->data);
        for (size_t i = 1; i < head->size; ++i) {
            head->data[i - 1] = std::move(head->data[i]);
        }
        --head->size;
        --size_;

        if (head->size == 0) {
            Node* next = head->next;
            destroy_node(head);
            head = next;
            if (!head) tail = nullptr;
        }
    }

    // Resize
    void resize(size_type count) {
        while (size_ > count) {
            pop_back();
        }
        while (size_ < count) {
            emplace_back();
        }
    }

    void resize(size_type count, const value_type& value) {
        while (size_ > count) {
            pop_back();
        }
        while (size_ < count) {
            push_back(value);
        }
    }
    
    // Swap
    void swap(unrolled_list& other) noexcept {
        std::swap(head, other.head);
        std::swap(tail, other.tail);
        std::swap(size_, other.size_);
        std::swap(allocator, other.allocator);
        std::swap(node_allocator, other.node_allocator);
    }

    // Range operations
    template<typename InputIt>
    void assign_range(InputIt first, InputIt last) {
        clear();
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    template<typename Range>
    void prepend_range(Range&& range) {
        insert_range(begin(), std::forward<Range>(range));
    }

    template<typename Range>
    void append_range(Range&& range) {
        insert_range(end(), std::forward<Range>(range));
    }

    template<typename Range>
    void insert_range(const_iterator pos, Range&& range) {
        for (const auto& item : range) {
            pos = insert(pos, item);
            ++pos;
        }
    }
};

// Comparison oparetors
template<typename T, size_t N, typename A>
bool operator==(const unrolled_list<T, N, A>& lhs, const unrolled_list<T, N, A>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename T, size_t N, typename A>
bool operator!=(const unrolled_list<T, N, A>& lhs, const unrolled_list<T, N, A>& rhs) {
    return !(lhs == rhs);
}