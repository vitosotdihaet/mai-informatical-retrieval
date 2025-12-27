#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

#include "skiplist.hpp"
#include "hashmap.hpp"
#include "log.hpp"

namespace boolean_index
{

    template <typename DocId = uint32_t>
    class BooleanIndex
    {
    private:
        using SkipListType = skiplist::SkipList<DocId>;
        using HashMapType = hashmap::HashMap<std::string, std::unique_ptr<SkipListType>>;

        HashMapType index_;
        SkipListType all_documents_;
        std::size_t total_documents_;
        std::size_t max_responses_;

    public:
        BooleanIndex() : total_documents_(0), max_responses_(0) {}
        BooleanIndex(std::size_t max_responses) : max_responses_(max_responses) {}

        // Add a document with terms
        void add_document(DocId doc_id, const std::vector<std::string> &terms)
        {
            all_documents_.insert(doc_id);
            total_documents_++;

            for (const auto &term : terms)
            {
                auto *skip_list_ptr = index_.find(term);

                if (skip_list_ptr == nullptr)
                {
                    // Create new skip list for this term
                    auto new_skip_list = std::make_unique<SkipListType>();
                    new_skip_list->insert(doc_id);
                    index_[term] = std::move(new_skip_list);
                }
                else
                {
                    // Add to existing skip list
                    (*skip_list_ptr)->insert(doc_id);
                }
            }
        }

        // Remove a document from the index
        bool remove_document(DocId doc_id, const std::vector<std::string> &terms)
        {
            if (!all_documents_.search(doc_id))
            {
                return false;
            }

            for (const auto &term : terms)
            {
                auto *skip_list_ptr = index_.find(term);
                if (skip_list_ptr != nullptr)
                {
                    (*skip_list_ptr)->remove(doc_id);

                    // If skip list is now empty, we could remove the term entirely
                    // But let's keep it for simplicity (empty skip lists are okay)
                }
            }

            all_documents_.remove(doc_id);
            total_documents_--;
            return true;
        }

        // Get documents containing ALL terms
        std::vector<DocId> and_query(const std::vector<std::string> &terms) const
        {
            if (terms.empty())
            {
                return {};
            }

            // Find the smallest posting list to intersect with others
            std::vector<const SkipListType *> posting_lists;
            std::size_t min_size = std::numeric_limits<std::size_t>::max();
            const SkipListType *smallest_list = nullptr;

            for (const auto &term : terms)
            {
                const auto *skip_list_ptr = index_.find(term);
                if (skip_list_ptr == nullptr)
                {
                    // Term doesn't exist, AND query returns empty
                    return {};
                }

                const auto *skip_list = skip_list_ptr->get();
                posting_lists.push_back(skip_list);

                std::size_t size = skip_list->size();
                if (size < min_size)
                {
                    min_size = size;
                    smallest_list = skip_list;
                }
            }

            // Intersect using the smallest list as base
            std::vector<DocId> result;

            for (const auto &doc_id : *smallest_list)
            {
                bool in_all = true;

                for (const auto *other_list : posting_lists)
                {
                    if (other_list == smallest_list)
                        continue;

                    if (!other_list->search(doc_id))
                    {
                        in_all = false;
                        break;
                    }
                }

                if (in_all)
                {
                    result.push_back(doc_id);
                    if (max_responses_ != 0 && result.size() >= max_responses_)
                    {
                        break;
                    }
                }
            }

            return result;
        }

        // Get documents containing ANY term (OR query)
        std::vector<DocId> or_query(const std::vector<std::string> &terms) const
        {
            if (terms.empty())
            {
                return {}; // Empty result for empty query
            }

            SkipListType union_result;

            for (const auto &term : terms)
            {
                const auto *skip_list_ptr = index_.find(term);
                if (skip_list_ptr == nullptr)
                {
                    continue; // Skip non-existent terms
                }

                const auto *skip_list = skip_list_ptr->get();

                // Add all documents from this skip list to union
                for (const auto &doc_id : *skip_list)
                {
                    union_result.insert(doc_id);
                    if (max_responses_ != 0 && union_result.size() >= max_responses_)
                    {
                        break;
                    }
                }
            }

            // Convert skip list to vector
            std::vector<DocId> result;
            for (const auto &doc_id : union_result)
            {
                result.push_back(doc_id);
            }

            return result;
        }

        // Get documents for a single term
        std::vector<DocId> get_documents_for_term(const std::string &term) const
        {
            const auto *skip_list_ptr = index_.find(term);
            if (skip_list_ptr == nullptr)
            {
                return {};
            }

            std::vector<DocId> result;
            const auto *skip_list = skip_list_ptr->get();
            for (const auto &doc_id : *skip_list)
            {
                result.push_back(doc_id);
            }
            return result;
        }

        // Get all terms in the index
        std::vector<std::string> get_all_terms() const
        {
            std::vector<std::string> terms;
            for (const auto &kv : index_)
            {
                terms.push_back(kv.first);
            }
            return terms;
        }

        // Get all documents in the index
        std::vector<DocId> get_all_documents() const
        {
            std::vector<DocId> result;
            for (const auto &doc_id : all_documents_)
            {
                result.push_back(doc_id);
            }
            return result;
        }

        // Check if a term exists in the index
        bool contains_term(const std::string &term) const
        {
            return index_.find(term) != nullptr;
        }

        // Check if a document exists in the index
        bool contains_document(DocId doc_id) const
        {
            return all_documents_.search(doc_id);
        }

        // Get number of documents containing a term
        std::size_t get_term_frequency(const std::string &term) const
        {
            const auto *skip_list_ptr = index_.find(term);
            if (skip_list_ptr == nullptr)
            {
                return 0;
            }
            return skip_list_ptr->get()->size();
        }

        // Get total number of documents
        std::size_t total_documents() const
        {
            return total_documents_;
        }

        // Get number of unique terms
        std::size_t total_terms() const
        {
            return index_.size();
        }

        // Print statistics about the index
        void print_statistics() const
        {
            std::cout << "Boolean Index Statistics:\n";
            std::cout << "  Total documents: " << total_documents_ << "\n";
            std::cout << "  Unique terms: " << index_.size() << "\n";
            std::cout << "  Memory usage (estimated):\n";

            // Estimate memory usage
            std::size_t hashmap_memory = index_.size() * sizeof(DocId) + index_.bucket_count() * sizeof(void *);

            std::size_t skiplist_memory = 0;
            std::size_t max_list_size = 0;
            std::size_t min_list_size = std::numeric_limits<std::size_t>::max();
            std::string largest_term, smallest_term;

            for (const auto &kv : index_)
            {
                std::size_t list_size = kv.second->size();
                skiplist_memory += list_size * sizeof(DocId) * 2;

                if (list_size > max_list_size)
                {
                    max_list_size = list_size;
                    largest_term = kv.first;
                }
                if (list_size < min_list_size)
                {
                    min_list_size = list_size;
                    smallest_term = kv.first;
                }
            }

            std::cout << "    HashMap: ~" << hashmap_memory / 1024 << " KB\n";
            std::cout << "    SkipLists: ~" << skiplist_memory / 1024 << " KB\n";
            std::cout << "    Total: ~" << (hashmap_memory + skiplist_memory) / 1024 << " KB\n";
            std::cout << "  Term statistics:\n";
            std::cout << "    Largest term: '" << largest_term
                      << "' (" << max_list_size << " documents)\n";
            std::cout << "    Smallest term: '" << smallest_term
                      << "' (" << min_list_size << " documents)\n";
        }

        // Print the entire index (for debugging)
        void print_index() const
        {
            std::cout << "Boolean Index Contents (first 10)\n";

            size_t counter = 0;
            for (const auto &kv : index_)
            {
                const auto &term = kv.first;
                const auto &skip_list = kv.second;

                std::cout << "\nTerm: '" << term << "' (" << skip_list->size() << " documents)\n";
                std::cout << "  Documents: ";

                std::size_t count = 0;
                for (const auto &doc_id : *skip_list)
                {
                    if (count > 10)
                    { // Limit output
                        std::cout << "... and " << (skip_list->size() - 10) << " more";
                        break;
                    }
                    std::cout << doc_id << " ";
                    count++;
                }
                std::cout << "\n";
                counter++;
                if (counter >= 10)
                {
                    break;
                }
            }

            std::cout << "\nTotal: " << index_.size() << " terms, "
                      << total_documents_ << " documents\n";
        }
    };

} // namespace boolean_index
