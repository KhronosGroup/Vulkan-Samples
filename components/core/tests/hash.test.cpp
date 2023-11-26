#include <catch2/catch_test_macros.hpp>

#include <core/util/hash.hpp>

std::unordered_map<std::string, std::string> hashes = {
    {"The quick brown fox jumps over the lazy dog", "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"},
    {"Hello World", "a591a6d40bf420404a011733cfb7b190d62c65bf0bcda32b57b277d9ad9f146e"},
    {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
    {"\n", "01ba4719c80b6fe911b091a7c05124b64eeece964e09c058ef8f9805daca546b"},
};

TEST_CASE("sha256", "[hash]")
{
	for (auto &[test, expected] : hashes)
	{
		std::string actual = vkb::sha256(test);
		REQUIRE(expected == actual);
	}
}