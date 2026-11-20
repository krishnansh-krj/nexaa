#pragma once

// NEXA DSA #5 - Multi-Linked (Orthogonal) List
// Many-to-many incident <-> resource deployment relationships.
// Every relationship node is threaded into TWO lists at once:
//   - the row list of its incident  (all resources deployed to it)
//   - the column list of its resource (all incidents it serves)
// Header lookup O(r) / O(c); linking a relationship O(1) after lookup;
// traversing one incident or one resource O(degree).

#include <cstddef>

namespace nexa::dsa
{

template <typename Payload>
class MultiLinkedList
{
public:
    struct Cell
    {
        unsigned long long rowKey;
        unsigned long long colKey;
        Payload payload;
        Cell* nextInRow;
        Cell* nextInCol;
    };

private:
    struct Header
    {
        unsigned long long key;
        Cell* first;
        std::size_t degree;
        Header* next;
    };

    Header* rows_ = nullptr;
    Header* cols_ = nullptr;
    std::size_t cells_ = 0;
    std::size_t rowCount_ = 0;
    std::size_t colCount_ = 0;

    static Header* find(Header* list, unsigned long long key)
    {
        for (Header* h = list; h != nullptr; h = h->next)
            if (h->key == key)
                return h;
        return nullptr;
    }

    static Header* findOrCreate(Header*& list, unsigned long long key, std::size_t& counter)
    {
        if (Header* h = find(list, key))
            return h;
        list = new Header{key, nullptr, 0, list};
        ++counter;
        return list;
    }

public:
    MultiLinkedList() = default;
    ~MultiLinkedList() { clear(); }

    MultiLinkedList(const MultiLinkedList&) = delete;
    MultiLinkedList& operator=(const MultiLinkedList&) = delete;

    void link(unsigned long long rowKey, unsigned long long colKey, const Payload& payload)
    {
        Header* row = findOrCreate(rows_, rowKey, rowCount_);
        Header* col = findOrCreate(cols_, colKey, colCount_);

        Cell* cell = new Cell{rowKey, colKey, payload, row->first, col->first};
        row->first = cell;
        col->first = cell;
        ++row->degree;
        ++col->degree;
        ++cells_;
    }

    template <typename Visitor>
    void forEachInRow(unsigned long long rowKey, Visitor visit) const
    {
        if (Header* row = find(rows_, rowKey))
            for (Cell* c = row->first; c != nullptr; c = c->nextInRow)
                visit(*c);
    }

    template <typename Visitor>
    void forEachInColumn(unsigned long long colKey, Visitor visit) const
    {
        if (Header* col = find(cols_, colKey))
            for (Cell* c = col->first; c != nullptr; c = c->nextInCol)
                visit(*c);
    }

    template <typename Visitor>
    void forEachRowKey(Visitor visit) const
    {
        for (Header* h = rows_; h != nullptr; h = h->next)
            visit(h->key, h->degree);
    }

    template <typename Visitor>
    void forEachColumnKey(Visitor visit) const
    {
        for (Header* h = cols_; h != nullptr; h = h->next)
            visit(h->key, h->degree);
    }

    std::size_t rowDegree(unsigned long long rowKey) const
    {
        Header* h = find(rows_, rowKey);
        return h ? h->degree : 0;
    }

    std::size_t columnDegree(unsigned long long colKey) const
    {
        Header* h = find(cols_, colKey);
        return h ? h->degree : 0;
    }

    std::size_t cellCount() const { return cells_; }
    std::size_t rowCount() const { return rowCount_; }
    std::size_t columnCount() const { return colCount_; }

    void clear()
    {
        // Each cell is owned once; free via the row lists.
        for (Header* h = rows_; h != nullptr;)
        {
            for (Cell* c = h->first; c != nullptr;)
            {
                Cell* next = c->nextInRow;
                delete c;
                c = next;
            }
            Header* next = h->next;
            delete h;
            h = next;
        }
        for (Header* h = cols_; h != nullptr;)
        {
            Header* next = h->next;
            delete h;
            h = next;
        }
        rows_ = cols_ = nullptr;
        cells_ = rowCount_ = colCount_ = 0;
    }
};

} // namespace nexa::dsa
