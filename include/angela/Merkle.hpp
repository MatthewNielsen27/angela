#pragma once

#include <cmath>
#include <compare>
#include <iomanip>
#include <istream>
#include <memory>
#include <vector>

#include "angela/Checksum.hpp"

namespace angela {

//! @short This struct is the data-node of the tree.
template <concepts::IsChecksum T>
struct MerkleNodeData {
    T checksum;
    std::size_t chunk_offset, chunk_size;

    auto operator<=>(const MerkleNodeData<T>&) const = default;
    bool operator==(const MerkleNodeData<T>&) const = default;
};

//! @short This struct is the inter-node relationship of the tree.
template <concepts::IsChecksum T>
struct MerkleNode {
    explicit MerkleNode(MerkleNodeData<T>&& d) : data{std::move(d)} {}

    MerkleNodeData<T> data;

    std::weak_ptr<MerkleNode> parent;
    std::shared_ptr<MerkleNode> left;
    std::shared_ptr<MerkleNode> right;

    [[nodiscard]] bool isRoot() const { return !parent.lock(); }
    [[nodiscard]] bool isLeaf() const { return !left && !right; }

    bool operator==(const MerkleNode& other) const {
        // todo: see what this would be like if I used the default generated comparison operators...
        if (isRoot() != other.isRoot()) { return false; }
        if (isLeaf() != other.isLeaf()) { return false; }

        return data == other.data;
    }
};

template <concepts::IsChecksumAlgo Algo = Sha256>
class MerkleTree {
public:
    using checksum_type = typename Algo::result_type;

    MerkleTree(std::string_view contents, std::size_t chunk_size) : m_ChunkSize(chunk_size) {
        auto stream = std::istringstream{std::string{contents}};
        make_tree(generate_leaves(stream));
    }

    MerkleTree(std::istringstream& stream, std::size_t chunk_size) : m_ChunkSize(chunk_size) {
        make_tree(generate_leaves(stream));
    }

    [[nodiscard]] bool operator==(const MerkleTree& other) const { return (root && other.root) && *root == *other.root; }

private:
    [[nodiscard]] std::vector<std::shared_ptr<MerkleNode<checksum_type>>> generate_leaves(std::istringstream& stream);
    void make_tree(std::vector<std::shared_ptr<MerkleNode<checksum_type>>>&& leaves);

    std::size_t m_ChunkSize;

    std::shared_ptr<MerkleNode<checksum_type>> root;
};

template <concepts::IsChecksumAlgo Algo>
std::vector<std::shared_ptr<MerkleNode<typename MerkleTree<Algo>::checksum_type>>> MerkleTree<Algo>::generate_leaves(std::istringstream& stream) {
    // pre-allocate a buffer to store data from
    std::vector<char> buf(m_ChunkSize, 0);

    std::vector<std::shared_ptr<MerkleNode<checksum_type>>> result;

    std::size_t read_offset = 0;

    while (!stream.eof()) {
        std::fill(buf.begin(), buf.end(), 0);
        stream.read(buf.data(), static_cast<std::streamsize>(m_ChunkSize));

        if (const auto bytes_read = static_cast<std::size_t>(stream.gcount())) {
            result.push_back(
                    std::make_shared<MerkleNode<checksum_type>>(
                            MerkleNodeData<checksum_type> {
                                    .checksum = Algo::hash(std::string_view(buf.data(), bytes_read)),
                                    .chunk_offset = read_offset,
                                    .chunk_size = bytes_read
                            }
                    )
            );

            read_offset += bytes_read;
        }
    }

    return result;
}

template <concepts::IsChecksumAlgo Algo>
void MerkleTree<Algo>::make_tree(std::vector<std::shared_ptr<MerkleNode<checksum_type>>>&& leaves) {
    using Level = std::vector<std::shared_ptr<MerkleNode<checksum_type>>>;

    // We need to store references to the tree nodes as we build up the tree
    std::vector<Level> tree = {leaves};

    const auto tree_height = std::log2(leaves.size());

    for (auto _i = 0; _i < tree_height; ++_i) {
        Level level;

        const auto& prev = tree.back();

        for (auto i = 0; i < prev.size() - 1; i+=2) {
            const auto& data_l = prev[i]->data.checksum.data;
            const auto& data_r = prev[i+1]->data.checksum.data;

            // concatenate the ranges ...
            std::vector<std::uint8_t> chunk;
            std::copy(data_l.begin(), data_l.end(), std::back_inserter(chunk));
            std::copy(data_r.begin(), data_r.end(), std::back_inserter(chunk));

            // ... and make a parent node using their checksums
            auto node = std::make_shared<MerkleNode<checksum_type>>(
                    MerkleNodeData<checksum_type> {
                            .checksum = Algo::hash(std::string(chunk.begin(), chunk.end())),
                            .chunk_offset = prev[i]->data.chunk_offset,
                            .chunk_size  = prev[i]->data.chunk_size + prev[i+1]->data.chunk_size
                    }
            );

            node->left = prev[i];
            node->right = prev[i+1];

            level.push_back(node);
        }

        if (prev.size() % 2) {
            level.push_back(prev.back());
        }

        tree.push_back(level);
    }

    // Sanity check that the tree is only 1
    assert(tree.back().size() == 1);

    root = tree.back().front();
}

} // namespace angela
