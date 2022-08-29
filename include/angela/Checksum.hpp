#pragma once

#include <cstdlib>
#include <array>
#include <iomanip>
#include <sstream>
#include <compare>
#include <concepts>

#include <openssl/sha.h>

namespace angela {

namespace concepts {

template <typename T>
concept IsChecksum = requires(T t) {
    // Has a member function equivalent to this
    { t.hexDigest() } -> std::same_as<std::string>;
};

template <typename T>
concept IsChecksumAlgo = requires(T t, std::string_view sv) {
    // Has a known result_type parameter
    typename T::result_type;

    // The result_type should satisfy IsChecksum
    IsChecksum<typename T::result_type>;

    // Has a static member function equivalent to this
    { T::hash(sv) } -> std::same_as<typename T::result_type>;
};

} // angela::concepts

template <std::size_t Bits, typename AlgoTag>
struct ChecksumBase {
    static constexpr auto size_bytes = Bits / 8;

    std::array<uint8_t, size_bytes> data = {0};

    auto operator<=>(const ChecksumBase&) const = default;
    bool operator==(const ChecksumBase&) const = default;

    [[nodiscard]] std::string hexDigest() const {
        std::stringstream ss;
        for (const auto& d : data) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(d);
        }
        return ss.str();
    }
};

//! @short This class defines a Sha256 hash algorithm.
struct Sha256 {
    using result_type = ChecksumBase<256, struct Sha256Tag>;

    static_assert(result_type::size_bytes == SHA256_DIGEST_LENGTH, "digest size mismatch");

    [[nodiscard]] static result_type hash(std::string_view contents) {
        result_type result{};

        {
            SHA256_CTX sha256;
            SHA256_Init(&sha256);
            SHA256_Update(&sha256, contents.data(), contents.size());
            SHA256_Final(result.data.data(), &sha256);
        }

        return result;
    }
};

} // namespace angela
