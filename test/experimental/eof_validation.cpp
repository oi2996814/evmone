
#include <doctest/doctest.h>
#include <evmc/hex.hpp>

namespace eof
{
using bytes_view = std::basic_string_view<uint8_t>;

constexpr uint8_t FORMAT = 0xef;
constexpr uint8_t MAGIC[] = {0xa6, 0x1c};
constexpr uint8_t TERMINATOR = 0x00;
constexpr uint8_t CODE_SECTION = 0x01;
constexpr uint8_t DATA_SECTION = 0x02;

enum class error_code
{
    success,
    starts_with_format,
    invalid_eof_prefix,
    eof_version_mismatch,
    eof_version_unknown,
};

error_code validate(bytes_view code, int expected_version);
error_code validate_eof1(bytes_view code_without_prefix);

error_code validate(bytes_view code, int expected_version)
{
    if (expected_version == 0)
        return (code.empty() || code[0] != FORMAT) ? error_code::success :
                                                     error_code::starts_with_format;

    if (code.size() < 4)
        return error_code::invalid_eof_prefix;

    if (code[0] != FORMAT || code[1] != MAGIC[0] || code[2] != MAGIC[1])
        return error_code::invalid_eof_prefix;

    const auto version = code[3];
    if (version != expected_version)
        return error_code::eof_version_mismatch;

    auto code_without_prefix = code;
    code_without_prefix.remove_prefix(4);
    switch (version)
    {
    default:
        return error_code::eof_version_unknown;
    case 1:
        return validate_eof1(code_without_prefix);
    }
}

error_code validate_eof1(bytes_view code_without_prefix)
{
    (void)code_without_prefix;
    return error_code::success;
}
}  // namespace eof

using namespace eof;
using evmc::from_hex;

TEST_CASE("validate empty code")
{
    CHECK(validate({}, 0) == error_code::success);
    CHECK(validate({}, 1) == error_code::invalid_eof_prefix);
    CHECK(validate({}, 2) == error_code::invalid_eof_prefix);
}

TEST_CASE("reject code starting with FORMAT in intermediate period")
{
    CHECK(validate(from_hex("00"), 0) == error_code::success);
    CHECK(validate(from_hex("FE"), 0) == error_code::success);
    CHECK(validate(from_hex("EF"), 0) == error_code::starts_with_format);
}

TEST_CASE("validate EOF prefix")
{
    CHECK(validate(from_hex("EFA61C01"), 1) == error_code::success);

    CHECK(validate(from_hex(""), 1) == error_code::invalid_eof_prefix);
    CHECK(validate(from_hex("EF"), 1) == error_code::invalid_eof_prefix);
    CHECK(validate(from_hex("EFA6"), 1) == error_code::invalid_eof_prefix);
    CHECK(validate(from_hex("EFA61C"), 1) == error_code::invalid_eof_prefix);

    CHECK(validate(from_hex("EEA61C01"), 1) == error_code::invalid_eof_prefix);
    CHECK(validate(from_hex("EFA71C01"), 1) == error_code::invalid_eof_prefix);
    CHECK(validate(from_hex("EFA61D01"), 1) == error_code::invalid_eof_prefix);
}

TEST_CASE("validate EOF version")
{
    CHECK(validate(from_hex("EFA61C01"), 1) == error_code::success);
    CHECK(validate(from_hex("EFA61C02"), 2) == error_code::eof_version_unknown);
    CHECK(validate(from_hex("EFA61CFF"), 0xff) == error_code::eof_version_unknown);

    CHECK(validate(from_hex("EFA61C01"), 2) == error_code::eof_version_mismatch);
    CHECK(validate(from_hex("EFA61C02"), 1) == error_code::eof_version_mismatch);
}