#pragma once

#include <vector>
#include <list>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace hashmap
{

    template <typename Key, typename Value, typename Hash = std::hash<Key>>
    class HashMap
    {
    private:
        static constexpr size_t initial_capacity_ = 16;
        static constexpr float max_load_factor_ = 0.75f;
        static constexpr size_t growth_factor_ = 2;

        struct KeyValuePair
        {
            Key key;
            Value value;

            template <typename K, typename V>
            KeyValuePair(K &&k, V &&v) : key(std::forward<K>(k)), value(std::forward<V>(v)) {}
        };

        using Bucket = std::list<KeyValuePair>;
        std::vector<Bucket> buckets_;
        size_t size_;
        Hash hash_function_;

        size_t bucket_index(const Key &key) const
        {
            return hash_function_(key) % buckets_.capacity();
        }

        typename Bucket::iterator find_in_bucket(size_t index, const Key &key)
        {
            auto &bucket = buckets_[index];
            return std::find_if(bucket.begin(), bucket.end(),
                                [&](const KeyValuePair &kv)
                                { return kv.key == key; });
        }

        typename Bucket::const_iterator find_in_bucket(size_t index, const Key &key) const
        {
            const auto &bucket = buckets_[index];
            return std::find_if(bucket.begin(), bucket.end(),
                                [&](const KeyValuePair &kv)
                                { return kv.key == key; });
        }

        void rehash_if_needed()
        {
            float load_factor = static_cast<float>(size_) / buckets_.capacity();
            if (load_factor > max_load_factor_)
            {
                rehash(buckets_.capacity() * growth_factor_);
            }
        }

        void rehash(size_t new_capacity)
        {
            std::vector<Bucket> new_buckets(new_capacity);

            for (auto &bucket : buckets_)
            {
                for (auto &kv : bucket)
                {
                    size_t new_index = hash_function_(kv.key) % new_capacity;
                    new_buckets[new_index].emplace_back(
                        std::move(kv.key),
                        std::move(kv.value));
                }
            }

            buckets_ = std::move(new_buckets);
        }

    public:
        HashMap() : buckets_(initial_capacity_), size_(0) {}

        explicit HashMap(size_t initial_capacity) : buckets_(initial_capacity), size_(0) {}

        HashMap(std::initializer_list<std::pair<Key, Value>> init_list)
            : buckets_(std::max(initial_capacity_, init_list.size() * 2)), size_(0)
        {
            for (const auto &pair : init_list)
            {
                insert(pair.first, pair.second);
            }
        }

        // Copy constructor
        HashMap(const HashMap &other)
            : buckets_(other.buckets_),
              size_(other.size_),
              hash_function_(other.hash_function_) {}

        // Move constructor
        HashMap(HashMap &&other) noexcept
            : buckets_(std::move(other.buckets_)),
              size_(other.size_),
              hash_function_(std::move(other.hash_function_))
        {
            other.size_ = 0;
        }

        // Copy assignment operator
        HashMap &operator=(const HashMap &other)
        {
            if (this != &other)
            {
                buckets_ = other.buckets_;
                size_ = other.size_;
                hash_function_ = other.hash_function_;
            }
            return *this;
        }

        // Move assignment operator
        HashMap &operator=(HashMap &&other) noexcept
        {
            if (this != &other)
            {
                buckets_ = std::move(other.buckets_);
                size_ = other.size_;
                hash_function_ = std::move(other.hash_function_);
                other.size_ = 0;
            }
            return *this;
        }

        template <typename V>
        bool insert(const Key &key, V &&value)
        {
            rehash_if_needed();

            size_t index = bucket_index(key);
            auto it = find_in_bucket(index, key);

            if (it != buckets_[index].end())
            {
                it->value = std::forward<V>(value); // move or copy
                return false;
            }

            buckets_[index].emplace_back(key, std::forward<V>(value));
            size_++;
            return true;
        }

        bool erase(const Key &key)
        {
            size_t index = bucket_index(key);
            auto it = find_in_bucket(index, key);

            if (it == buckets_[index].end())
            {
                return false;
            }

            buckets_[index].erase(it);
            size_--;
            return true;
        }

        Value *find(const Key &key)
        {
            size_t index = bucket_index(key);
            auto it = find_in_bucket(index, key);

            if (it == buckets_[index].end())
            {
                return nullptr;
            }

            return &(it->value);
        }

        const Value *find(const Key &key) const
        {
            size_t index = bucket_index(key);
            auto it = find_in_bucket(index, key);

            if (it == buckets_[index].end())
            {
                return nullptr;
            }

            return &(it->value);
        }

        Value &operator[](const Key &key)
        {
            size_t index = bucket_index(key);
            auto it = find_in_bucket(index, key);

            if (it == buckets_[index].end())
            {
                rehash_if_needed();
                index = bucket_index(key);
                buckets_[index].emplace_back(key, Value{}); // nullptr for unique_ptr
                size_++;
                return buckets_[index].back().value;
            }

            return it->value;
        }

        const Value &at(const Key &key) const
        {
            const Value *value = find(key);
            if (!value)
            {
                throw std::out_of_range("Key not found in HashMap");
            }
            return *value;
        }

        bool contains(const Key &key) const
        {
            return find(key) != nullptr;
        }

        size_t size() const
        {
            return size_;
        }

        bool empty() const
        {
            return size_ == 0;
        }

        void clear()
        {
            for (auto &bucket : buckets_)
            {
                bucket.clear();
            }
            size_ = 0;
        }

        float load_factor() const
        {
            return static_cast<float>(size_) / buckets_.capacity();
        }

        size_t bucket_count() const
        {
            return buckets_.capacity();
        }

        size_t bucket_size(size_t index) const
        {
            if (index >= buckets_.capacity())
            {
                throw std::out_of_range("Bucket index out of range");
            }
            return buckets_[index].size();
        }

        void print_stats() const
        {
            std::cout << "HashMap Statistics:\n";
            std::cout << "  Size: " << size_ << "\n";
            std::cout << "  Bucket count: " << bucket_count() << "\n";
            std::cout << "  Load factor: " << load_factor() << "\n";

            size_t max_bucket_size = 0;
            size_t empty_buckets = 0;

            for (size_t i = 0; i < bucket_count(); ++i)
            {
                size_t bucket_size = buckets_[i].size();
                max_bucket_size = std::max(max_bucket_size, bucket_size);
                if (bucket_size == 0)
                {
                    empty_buckets++;
                }
            }

            std::cout << "  Max bucket size: " << max_bucket_size << "\n";
            std::cout << "  Empty buckets: " << empty_buckets << "\n";
            std::cout << "  Bucket utilization: "
                      << (1.0f - static_cast<float>(empty_buckets) / bucket_count()) * 100.0f
                      << "%\n";
        }

        class Iterator
        {
        private:
            typename std::vector<Bucket>::iterator outer_it_;
            typename std::vector<Bucket>::iterator outer_end_;
            typename Bucket::iterator inner_it_;

            void find_next_non_empty()
            {
                while (outer_it_ != outer_end_ && inner_it_ == outer_it_->end())
                {
                    ++outer_it_;
                    if (outer_it_ != outer_end_)
                    {
                        inner_it_ = outer_it_->begin();
                    }
                }
            }

        public:
            Iterator(std::vector<Bucket> &buckets, bool end = false)
                : outer_it_(end ? buckets.end() : buckets.begin()),
                  outer_end_(buckets.end())
            {

                if (!end && outer_it_ != outer_end_)
                {
                    inner_it_ = outer_it_->begin();
                    find_next_non_empty();
                }
            }

            Iterator &operator++()
            {
                if (outer_it_ == outer_end_)
                {
                    return *this;
                }

                ++inner_it_;
                find_next_non_empty();
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator temp = *this;
                ++(*this);
                return temp;
            }

            std::pair<const Key &, Value &> operator*()
            {
                return {inner_it_->key, inner_it_->value};
            }

            std::pair<const Key *, Value *> operator->()
            {
                return {&inner_it_->key, &inner_it_->value};
            }

            bool operator==(const Iterator &other) const
            {
                if (outer_it_ != other.outer_it_)
                {
                    return false;
                }

                if (outer_it_ == outer_end_ || outer_it_ == other.outer_end_)
                {
                    return true;
                }

                return inner_it_ == other.inner_it_;
            }

            bool operator!=(const Iterator &other) const
            {
                return !(*this == other);
            }
        };

        class ConstIterator
        {
        private:
            typename std::vector<Bucket>::const_iterator outer_it_;
            typename std::vector<Bucket>::const_iterator outer_end_;
            typename Bucket::const_iterator inner_it_;

            void find_next_non_empty()
            {
                while (outer_it_ != outer_end_ && inner_it_ == outer_it_->end())
                {
                    ++outer_it_;
                    if (outer_it_ != outer_end_)
                    {
                        inner_it_ = outer_it_->begin();
                    }
                }
            }

        public:
            ConstIterator(const std::vector<Bucket> &buckets, bool end = false)
                : outer_it_(end ? buckets.cend() : buckets.cbegin()),
                  outer_end_(buckets.cend())
            {

                if (!end && outer_it_ != outer_end_)
                {
                    inner_it_ = outer_it_->begin();
                    find_next_non_empty();
                }
            }

            ConstIterator &operator++()
            {
                if (outer_it_ == outer_end_)
                {
                    return *this;
                }

                ++inner_it_;
                find_next_non_empty();
                return *this;
            }

            ConstIterator operator++(int)
            {
                ConstIterator temp = *this;
                ++(*this);
                return temp;
            }

            const std::pair<const Key &, const Value &> operator*() const
            {
                return {inner_it_->key, inner_it_->value};
            }

            const std::pair<const Key *, const Value *> operator->() const
            {
                return {&inner_it_->key, &inner_it_->value};
            }

            bool operator==(const ConstIterator &other) const
            {
                if (outer_it_ != other.outer_it_)
                {
                    return false;
                }

                if (outer_it_ == outer_end_ || outer_it_ == other.outer_end_)
                {
                    return true;
                }

                return inner_it_ == other.inner_it_;
            }

            bool operator!=(const ConstIterator &other) const
            {
                return !(*this == other);
            }
        };

        Iterator begin()
        {
            return Iterator(buckets_);
        }

        Iterator end()
        {
            return Iterator(buckets_, true);
        }

        ConstIterator begin() const
        {
            return ConstIterator(buckets_);
        }

        ConstIterator end() const
        {
            return ConstIterator(buckets_, true);
        }

        ConstIterator cbegin() const
        {
            return ConstIterator(buckets_);
        }

        ConstIterator cend() const
        {
            return ConstIterator(buckets_, true);
        }
    };

} // namespace hashmap