#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define _UTF8_CPP_TOOLS_HH_TEST_MODE
#include "doctest.h"
#include "../utf8_cpp_utils.hh"


TEST_CASE("Testing 'casefold' function") {
    CHECK( Utf8CppUtils::utf8::casefold("ABCabc") == "abcabc" );
    CHECK( Utf8CppUtils::utf8::casefold("XYZxyz") == "xyzxyz" );
    CHECK( Utf8CppUtils::utf8::casefold("ÄÖÜ") == "äöü" );
    CHECK( Utf8CppUtils::utf8::casefold("ß") == "ss" );
    CHECK( Utf8CppUtils::utf8::casefold("Ǉ") == "ǉ" );
}

TEST_CASE("Testing '_decode_utf8' function") {
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("A")) == 0x41 );
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("€")) == 0x20ac );
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("Ša")) == 0x160 );                  // only the first letter is checked
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("©")) == 0xa9 );
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("Ä")) == 0xc4 );
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("\xc5Ä"), 1) == 0xc4 );             // check from offset 1, omitting the first octet
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("Ʃ")) == 0x1a9 );
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("\xe0\xa2\xa5")) == 0x8a5 );        // Arabic Letter Qaf With Dot Below: ࢥ
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("\xe0\xb7\x92")) == 0xdd2 );        // Sinhala Vowel Sign Ketti Is-pilla: ි
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("\xf0\x91\x84\x85")) == 0x11105 );  // Chakma Letter U: 𑄅
    CHECK( Utf8CppUtils::utf8::_decode_utf8_str(std::string("𝄞")) == 0x1D11E );                 // Violinschlüssel: 𝄞
}

TEST_CASE("Testing 'casefold_compare' function") {
    // CHECK( Utf8CppUtils::utf8::casefold_compare("abc", "abc") == 0 );
    // CHECK( Utf8CppUtils::utf8::casefold_compare("ABC", "abc") == 0 );
    // CHECK( Utf8CppUtils::utf8::casefold_compare("abc", "abd") < 0 );
    // CHECK( Utf8CppUtils::utf8::casefold_compare("abc", "abb") > 0 );
    // CHECK( Utf8CppUtils::utf8::casefold_compare("ÄÖÜ", "ÄÖÜ") == 0 );
    // CHECK( Utf8CppUtils::utf8::casefold_compare("ÄÖÜ", "äöü") == 0 );
    // CHECK( Utf8CppUtils::utf8::casefold_compare("Öß", "öss") == 0 );

    // CHECK( Utf8CppUtils::utf8::casefold_compare("ÜÖÄ", "üöa") < 0 );
    // CHECK( Utf8CppUtils::utf8::casefold_compare("ÜÖÄ", "üöü") > 0 );
}
