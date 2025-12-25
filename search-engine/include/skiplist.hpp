#pragma once

#include <iostream>
#include <vector>
#include <random>
#include <memory>
#include <limits>
#include <algorithm>

namespace skiplist
{

    template <typename T>
    struct SkipListNode
    {
        T value;
        std::vector<std::shared_ptr<SkipListNode<T>>> forward;
        int level;

        SkipListNode(T val, int lvl) : value(val), level(lvl)
        {
            forward.resize(lvl + 1, nullptr);
        }
    };

    template <typename T>
    class SkipList
    {
    private:
        std::shared_ptr<SkipListNode<T>> header_;
        int max_level_;
        int current_level_;
        float probability_;
        std::mt19937 generator_;
        std::uniform_real_distribution<float> distribution_;

        int random_level()
        {
            float r = distribution_(generator_);
            int lvl = 0;
            while (r < probability_ && lvl < max_level_)
            {
                lvl++;
                r = distribution_(generator_);
            }
            return lvl;
        }

    public:
        SkipList(int max_level = 16, float prob = 0.5)
            : max_level_(max_level), current_level_(0), probability_(prob),
              generator_(std::random_device{}()), distribution_(0.0f, 1.0f)
        {

            T min_value = std::numeric_limits<T>::lowest();
            header_ = std::make_shared<SkipListNode<T>>(min_value, max_level_);
        }

        bool insert(const T &value)
        {
            std::vector<std::shared_ptr<SkipListNode<T>>> update(max_level_ + 1, nullptr);
            auto current = header_;

            for (int i = current_level_; i >= 0; i--)
            {
                while (current->forward[i] != nullptr &&
                       current->forward[i]->value < value)
                {
                    current = current->forward[i];
                }
                update[i] = current;
            }

            current = current->forward[0];

            if (current != nullptr && current->value == value)
            {
                return false;
            }

            int new_level = random_level();

            if (new_level > current_level_)
            {
                for (int i = current_level_ + 1; i <= new_level; i++)
                {
                    update[i] = header_;
                }
                current_level_ = new_level;
            }

            auto new_node = std::make_shared<SkipListNode<T>>(value, new_level);

            for (int i = 0; i <= new_level; i++)
            {
                new_node->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = new_node;
            }

            return true;
        }

        bool search(const T &value) const
        {
            auto current = header_;

            for (int i = current_level_; i >= 0; i--)
            {
                while (current->forward[i] != nullptr &&
                       current->forward[i]->value < value)
                {
                    current = current->forward[i];
                }
            }

            current = current->forward[0];
            return current != nullptr && current->value == value;
        }

        bool remove(const T &value)
        {
            std::vector<std::shared_ptr<SkipListNode<T>>> update(max_level_ + 1, nullptr);
            auto current = header_;

            for (int i = current_level_; i >= 0; i--)
            {
                while (current->forward[i] != nullptr &&
                       current->forward[i]->value < value)
                {
                    current = current->forward[i];
                }
                update[i] = current;
            }

            current = current->forward[0];

            if (current != nullptr && current->value == value)
            {
                for (int i = 0; i <= current->level; i++)
                {
                    if (update[i]->forward[i] != current)
                    {
                        break;
                    }
                    update[i]->forward[i] = current->forward[i];
                }

                while (current_level_ > 0 && header_->forward[current_level_] == nullptr)
                {
                    current_level_--;
                }

                return true;
            }

            return false;
        }

        T get_min() const
        {
            if (header_->forward[0] == nullptr)
            {
                throw std::runtime_error("Skip list is empty");
            }
            return header_->forward[0]->value;
        }

        T get_max() const
        {
            auto current = header_;

            for (int i = current_level_; i >= 0; i--)
            {
                while (current->forward[i] != nullptr)
                {
                    current = current->forward[i];
                }
            }

            if (current == header_)
            {
                throw std::runtime_error("Skip list is empty");
            }

            return current->value;
        }

        bool empty() const
        {
            return header_->forward[0] == nullptr;
        }

        size_t size() const
        {
            size_t count = 0;
            auto current = header_->forward[0];
            while (current != nullptr)
            {
                count++;
                current = current->forward[0];
            }
            return count;
        }

        void print() const
        {
            std::cout << "\nSkip List (Level " << current_level_ << "):\n";
            for (int i = current_level_; i >= 0; i--)
            {
                std::cout << "Level " << i << ": ";
                auto current = header_->forward[i];
                while (current != nullptr)
                {
                    std::cout << current->value << " ";
                    current = current->forward[i];
                }
                std::cout << "\n";
            }
            std::cout << std::endl;
        }

        class Iterator
        {
        private:
            std::shared_ptr<SkipListNode<T>> current_;

        public:
            Iterator(std::shared_ptr<SkipListNode<T>> node) : current_(node) {}

            T operator*() const
            {
                return current_->value;
            }

            Iterator &operator++()
            {
                if (current_)
                {
                    current_ = current_->forward[0];
                }
                return *this;
            }

            bool operator!=(const Iterator &other) const
            {
                return current_ != other.current_;
            }

            bool operator==(const Iterator &other) const
            {
                return current_ == other.current_;
            }
        };

        Iterator begin() const
        {
            return Iterator(header_->forward[0]);
        }

        Iterator end() const
        {
            return Iterator(nullptr);
        }
    };

} // namespace skiplist