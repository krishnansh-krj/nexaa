#pragma once

// NEXA DSA #27 - Trie (prefix tree, case-insensitive)
// Prefix search over incident codes / titles and resource codes / names.
// Children are kept in a sorted sibling list, so any character set works
// (codes contain '-', titles contain digits). insert / lookup O(L * sigma),
// prefix collect O(L + size of matching subtree).

#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace nexa::dsa
{

class Trie
{
public:
    struct Match
    {
        std::string word;
        unsigned long long id;
    };

private:
    struct Node
    {
        char ch = 0;
        Node* firstChild = nullptr;
        Node* nextSibling = nullptr;
        std::vector<unsigned long long> ids; // ids whose key ends here
    };

    Node* root_ = new Node;
    std::size_t words_ = 0;
    std::size_t nodes_ = 1;

    static char fold(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

    static void destroy(Node* n)
    {
        while (n)
        {
            destroy(n->firstChild);
            Node* next = n->nextSibling;
            delete n;
            n = next;
        }
    }

    static Node* child(const Node* n, char c)
    {
        for (Node* x = n->firstChild; x && x->ch <= c; x = x->nextSibling)
            if (x->ch == c)
                return x;
        return nullptr;
    }

    Node* childOrCreate(Node* n, char c)
    {
        Node** link = &n->firstChild;
        while (*link && (*link)->ch < c)
            link = &(*link)->nextSibling;
        if (*link && (*link)->ch == c)
            return *link;
        Node* created = new Node;
        created->ch = c;
        created->nextSibling = *link;
        *link = created;
        ++nodes_;
        return created;
    }

    const Node* walk(const std::string& key) const
    {
        const Node* n = root_;
        for (char c : key)
        {
            n = child(n, fold(c));
            if (!n)
                return nullptr;
        }
        return n;
    }

    static void collect(const Node* n, std::string& path, std::vector<Match>& out, std::size_t limit)
    {
        if (out.size() >= limit)
            return;
        for (unsigned long long id : n->ids)
        {
            if (out.size() >= limit) return;
            out.push_back(Match{path, id});
        }
        for (const Node* c = n->firstChild; c; c = c->nextSibling)
        {
            path.push_back(c->ch);
            collect(c, path, out, limit);
            path.pop_back();
            if (out.size() >= limit) return;
        }
    }

public:
    Trie() = default;
    ~Trie() { destroy(root_); }

    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insert(const std::string& key, unsigned long long id)
    {
        if (key.empty())
            return;
        Node* n = root_;
        for (char c : key)
            n = childOrCreate(n, fold(c));
        for (unsigned long long existing : n->ids)
            if (existing == id)
                return;
        n->ids.push_back(id);
        ++words_;
    }

    // Inserts every whitespace-separated token, plus the whole phrase.
    void insertPhrase(const std::string& phrase, unsigned long long id)
    {
        insert(phrase, id);
        std::string token;
        for (char c : phrase)
        {
            if (std::isspace(static_cast<unsigned char>(c)))
            {
                if (!token.empty()) insert(token, id);
                token.clear();
            }
            else
                token.push_back(c);
        }
        if (!token.empty() && token.size() != phrase.size())
            insert(token, id);
    }

    bool contains(const std::string& key) const
    {
        const Node* n = walk(key);
        return n && !n->ids.empty();
    }

    // Matches in lexicographic order of the stored (lower-cased) key.
    std::vector<Match> withPrefix(const std::string& prefix, std::size_t limit = 50) const
    {
        std::vector<Match> out;
        const Node* n = walk(prefix);
        if (!n || limit == 0)
            return out;
        std::string path;
        for (char c : prefix) path.push_back(fold(c));
        collect(n, path, out, limit);
        return out;
    }

    std::size_t wordCount() const { return words_; }
    std::size_t nodeCount() const { return nodes_; }
};

} // namespace nexa::dsa
