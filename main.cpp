#include <iostream>

#include "angela/Merkle.hpp"

int main() {
    const auto checksum = angela::Sha256::hash("hello world");
    const auto checksumString = checksum.hexDigest();

    const auto tree_1 = angela::MerkleTree{"hello world", 1};
    const auto tree_2 = angela::MerkleTree{"hello worlb", 1};

    std::cout << (tree_1 == tree_2) << std::endl;

    std::cout << checksumString << std::endl;

    return 0;
}
