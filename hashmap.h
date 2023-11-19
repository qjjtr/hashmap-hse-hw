#pragma once
#include <list>
#include <stdexcept>
#include <vector>

template<typename Key, typename Value, typename Hash = std::hash<Key>>
class HashMap {
private:
    inline static const size_t START_BUCKET_COUNT = 37;
    inline static const float MAX_LOAD_FACTOR = 0.6;

public:
    using NodeType = std::pair<const Key, Value>;

private:
    struct BucketItem {
        NodeType data;
        size_t distance_to_ideal;
        size_t id_in_table;
    public:
        BucketItem(NodeType data, size_t distance_to_ideal, size_t id_in_table)
            : data(data), distance_to_ideal(distance_to_ideal), id_in_table(id_in_table) {}
    };

    using ListIterator = typename std::list<BucketItem>::iterator;
    using ListConstIterator = typename std::list<BucketItem>::const_iterator;

    template<bool is_const>
    struct common_iterator {
        friend class HashMap<Key, Value, Hash>;
    private:
        using InnerListIterator = std::conditional_t<is_const, ListConstIterator, ListIterator>;
        InnerListIterator inner_iterator_;
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<is_const, const NodeType, NodeType>;
        using pointer = std::conditional_t<is_const, const NodeType*, NodeType*>;
        using reference = std::conditional_t<is_const, const NodeType&, NodeType&>;
        using iterator_category = std::forward_iterator_tag;

        common_iterator();
        explicit common_iterator(InnerListIterator it);
        common_iterator(const common_iterator<false>& it);
        bool operator==(const common_iterator<is_const>& x);
        bool operator!=(const common_iterator<is_const>& x);
        reference operator*();
        pointer operator->();
        common_iterator<is_const>& operator++();
        common_iterator<is_const> operator++(int);
    };

  public:
    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;

    HashMap(Hash hash = Hash{});
    HashMap(const HashMap<Key, Value, Hash>& another);
    HashMap<Key, Value, Hash>& operator=(const HashMap<Key, Value, Hash>& another);

    template<typename TIterator>
    HashMap(TIterator begin, TIterator end, Hash hash = Hash{});
    HashMap(std::initializer_list<std::pair<const Key, Value>> items, Hash hash = Hash{});

    size_t size() const;
    bool empty() const;

    const Hash& hash_function() const;

    void insert(const NodeType& x);

    void erase(const Key& key);
    template <bool is_const>
    void erase(common_iterator<is_const> iterator);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    iterator find(const Key& key);
    const_iterator find(const Key& key) const;

    Value& operator[](const Key& key);
    const Value& at(const Key& key) const;

    void clear();

private:
    void rehash_if_needed();

private:
    Hash hasher_;
    std::list<BucketItem> items_;
    std::vector<ListIterator> table_;

};

template<typename Key, typename Value, typename Hash>
HashMap<Key, Value, Hash>::HashMap(Hash hash)
    : hasher_(std::move(hash))
    , items_(std::list<BucketItem>{})
    , table_(std::vector<ListIterator>(START_BUCKET_COUNT, items_.end()))
{}

template<typename Key, typename Value, typename Hash>
template<typename TIterator>
HashMap<Key, Value, Hash>::HashMap(TIterator begin, TIterator end, Hash hash)
    : HashMap(hash)
{
    for (auto it = begin; it != end; ++it) {
        insert(*it);
    }
}

template<typename Key, typename Value, typename Hash>
HashMap<Key, Value, Hash>::HashMap(std::initializer_list<std::pair<const Key, Value>> items, Hash hash)
    : HashMap(hash)
{
    for (const auto& element : items) {
        insert(element);
    }
}

template<typename Key, typename Value, typename Hash>
HashMap<Key, Value, Hash>::HashMap(const HashMap<Key, Value, Hash>& another)
    : hasher_(another.hasher_)
    , items_(std::list<BucketItem>{})
    , table_(std::vector<ListIterator>(another.table_.size(), items_.end()))
{
    for (const auto& node : another) {
        insert(node);
    }
}

template<typename Key, typename Value, typename Hash>
HashMap<Key, Value, Hash>& HashMap<Key, Value, Hash>::operator=(const HashMap<Key, Value, Hash>& another) {
    if (&another == this) {
        return *this;
    }
    hasher_ = another.hasher_;
    items_.clear();
    table_.clear();
    table_.resize(another.table_.size(), items_.end());

    for (const auto& node : another) {
        insert(node);
    }
    return *this;
}

template<typename Key, typename Value, typename Hash>
size_t HashMap<Key, Value, Hash>::size() const {
    return items_.size();
}

template<typename Key, typename Value, typename Hash>
bool HashMap<Key, Value, Hash>::empty() const {
    return size() == 0;
}

template<typename Key, typename Value, typename Hash>
const Hash& HashMap<Key, Value, Hash>::hash_function() const {
    return hasher_;
}

template<typename Key, typename Value, typename Hash>
void HashMap<Key, Value, Hash>::insert(const NodeType& x) {
    if (find(x.first) != end()) {
        return;
    }
    rehash_if_needed();

    auto list_iterator = items_.emplace(items_.end(), x, 0, 0);
    size_t hash = hasher_(x.first) % table_.size();

    while (table_[hash] != items_.end()) {
        if (table_[hash]->distance_to_ideal < list_iterator->distance_to_ideal) {
            std::swap(list_iterator, table_[hash]);
            table_[hash]->id_in_table = hash;
        }
        ++list_iterator->distance_to_ideal;
        hash = (hash + 1) % table_.size();
    }
    table_[hash] = list_iterator;
    table_[hash]->id_in_table = hash;
}

template<typename Key, typename Value, typename Hash>
void HashMap<Key, Value, Hash>::erase(const Key& key) {
    auto it = find(key);
    if (it != end()) {
        erase(it);
    }
}

template<typename Key, typename Value, typename Hash>
template <bool is_const>
void HashMap<Key, Value, Hash>::erase(common_iterator<is_const> iterator) {
    auto inner_iter = iterator.inner_iterator_;
    size_t hash = inner_iter->id_in_table;
    items_.erase(inner_iter);
    table_[hash] = items_.end();
    size_t next_hash = (hash + 1) % table_.size();
    while (table_[next_hash] != items_.end() && table_[next_hash]->distance_to_ideal > 0) {
        std::swap(table_[hash], table_[next_hash]);
        table_[hash]->id_in_table = hash;
        --table_[hash]->distance_to_ideal;
        hash = next_hash;
        next_hash = (hash + 1) % table_.size();
    }
}

template<typename Key, typename Value, typename Hash>
typename HashMap<Key, Value, Hash>::iterator
        HashMap<Key, Value, Hash>::begin() {
    return iterator(items_.begin());
}

template<typename Key, typename Value, typename Hash>
typename HashMap<Key, Value, Hash>::iterator
        HashMap<Key, Value, Hash>::end() {
    return iterator(items_.end());
}

template<typename Key, typename Value, typename Hash>
typename HashMap<Key, Value, Hash>::const_iterator
        HashMap<Key, Value, Hash>::begin() const {
    return const_iterator(items_.begin());
}

template<typename Key, typename Value, typename Hash>
typename HashMap<Key, Value, Hash>::const_iterator
        HashMap<Key, Value, Hash>::end() const {
    return const_iterator(items_.end());
}

template<typename Key, typename Value, typename Hash>
typename HashMap<Key, Value, Hash>::const_iterator
        HashMap<Key, Value, Hash>::find(const Key& key) const
{
    size_t hash = hasher_(key) % table_.size();

    while (!(table_[hash] == items_.end() || table_[hash]->data.first == key)) {
        hash = (hash + 1) % table_.size();
    }
    return const_iterator(table_[hash]);
}

template<typename Key, typename Value, typename Hash>
typename HashMap<Key, Value, Hash>::iterator
        HashMap<Key, Value, Hash>::find(const Key& key)
{
    size_t hash = hasher_(key) % table_.size();

    while (!(table_[hash] == items_.end() || table_[hash]->data.first == key)) {
        hash = (hash + 1) % table_.size();
    }
    return iterator(table_[hash]);
}

template<typename Key, typename Value, typename Hash>
Value& HashMap<Key, Value, Hash>::operator[](const Key& key) {
    insert({key, Value{}});
    return find(key)->second;
}

template<typename Key, typename Value, typename Hash>
const Value& HashMap<Key, Value, Hash>::at(const Key& key) const {
    auto it = find(key);
    if (it == end()) {
        throw std::out_of_range("out of range");
    }
    return it->second;
}

template<typename Key, typename Value, typename Hash>
void HashMap<Key, Value, Hash>::clear() {
    auto list_it = items_.begin();
    while (list_it != items_.end()) {
        auto temp_it = list_it;
        ++list_it;
        erase(iterator{temp_it});
    }
}

template<typename Key, typename Value, typename Hash>
void HashMap<Key, Value, Hash>::rehash_if_needed() {
    if (static_cast<double>(size() + 1) / table_.size() < MAX_LOAD_FACTOR) {
        return;
    }

    size_t min_bucket_count = table_.size() * 2;
    for ( ; ; ++min_bucket_count) {
        bool prime = true;
        for (size_t div = 2; div * div <= min_bucket_count; ++div) {
            if (min_bucket_count % div == 0) {
                prime = false;
                break;
            }
        }
        if (prime) break;
    }

    auto old_items = std::move(items_);
    items_.clear();

    table_.clear();
    table_.resize(min_bucket_count, items_.end());

    for (const auto& item : old_items) {
        insert(item.data);
    }
}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
HashMap<Key, Value, Hash>::common_iterator<is_const>::common_iterator() {}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
HashMap<Key, Value, Hash>::common_iterator<is_const>::
        common_iterator(InnerListIterator it): inner_iterator_(it) {}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
HashMap<Key, Value, Hash>::common_iterator<is_const>::
        common_iterator(const common_iterator<false>& it): inner_iterator_(it.inner_iterator_) {}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
bool HashMap<Key, Value, Hash>::common_iterator<is_const>::
        operator==(const HashMap<Key, Value, Hash>::common_iterator<is_const>& x) {
    return inner_iterator_ == x.inner_iterator_;
}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
bool HashMap<Key, Value, Hash>::common_iterator<is_const>::
        operator!=(const HashMap<Key, Value, Hash>::common_iterator<is_const>& x) {
    return !operator==(x);
}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
typename HashMap<Key, Value, Hash>::template common_iterator<is_const>::reference
        HashMap<Key, Value, Hash>::common_iterator<is_const>::operator*() {
    return inner_iterator_->data;
}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
typename HashMap<Key, Value, Hash>::template common_iterator<is_const>::pointer
        HashMap<Key, Value, Hash>::common_iterator<is_const>::operator->() {
    return &inner_iterator_->data;
}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
HashMap<Key, Value, Hash>::template common_iterator<is_const>&
        HashMap<Key, Value, Hash>::common_iterator<is_const>::operator++() {
    ++inner_iterator_;
    return *this;
}

template<typename Key, typename Value, typename Hash>
template<bool is_const>
HashMap<Key, Value, Hash>::template common_iterator<is_const>
        HashMap<Key, Value, Hash>::common_iterator<is_const>::operator++(int) {
    HashMap<Key, Value, Hash>::common_iterator<is_const> ret = *this;
    ++inner_iterator_;
    return ret;
}
