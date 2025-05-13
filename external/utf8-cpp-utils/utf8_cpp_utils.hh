#ifndef _UTF8_CPP_TOOLS_HH
#define _UTF8_CPP_TOOLS_HH

#include <string>
#include <cstdint>

#include <iostream>

#include "utf8_cpp_tables.hh"


namespace Utf8CppUtils {

    class utf8 {

    public:

        // Function to perform casefolding replacement
        static std::string casefold(const std::string& input) {
            std::string output;
            size_t i = 0;
            
            while (i < input.size()) {
                uint8_t current_byte = static_cast<uint8_t>(input[i]);
                bool replaced = false;

                if( current_byte >= 'A' && current_byte <= 'Z' ) {
                    // Casefold uppercase ASCII characters
                    output.push_back( static_cast<char>(current_byte + 32) );                        // Convert to lowercase
                    i++;
                    continue;
                }
                // Check if the current byte is in the _casefold_table
                auto it = _casefold_table.find(current_byte);
                if (it != _casefold_table.end()) {
                    const auto& replacements = it->second;
                    
                    // Check if any of the stored sequences match
                    for (const auto& [from, to] : replacements) {
                        if (input.compare(i, from.size(), from) == 0) {         // Directly compare substring
                            output.append(to);
                            i += from.size();                                   // Move past the matched sequence
                            replaced = true;
                            break;
                        }
                    }
                }
                // If no replacement occurred, just copy the current byte
                if (!replaced) {
                    output.push_back(input[i]);
                    i++;
                }
            }
            
            return output;
        }


        // Not working – do we really need it!? Is it faster than casefolding and then comparing!?
        static int casefold_compare(const std::string& str1, const std::string& str2) {
            int compare = 0;
            uint8_t *ptr1 = (uint8_t *) str1.c_str();
            uint8_t *ptr2 = (uint8_t *) str2.c_str();
            uint8_t *end_ptr1 = ptr1 + str1.size() - 1;
            uint8_t *end_ptr2 = ptr2 + str2.size() - 1;

            std::cout << "--------------- " << str1 << " <> " << str2 << "|" << std::endl;
            while( ptr1 <= end_ptr1 && ptr2 <= end_ptr2 ) {
                std::cout << ptr1 << "." << (uint8_t) *(ptr1+1) << " | " << ptr2 << std::endl;
                if( *ptr1 <= 0x7f && *ptr2 <= 0x7f ) {
                    // ASCII: Most of the time we're comparing ASCII letters, so let's handle this simple case first
                    if( *ptr1 >= 'A' && *ptr1 <= 'Z' )
                        *ptr1 += 32;
                    if( *ptr2 >= 'A' && *ptr2 <= 'Z' )
                        *ptr2 += 32;
                    if( *ptr1 == *ptr2 ) {
                        ++ptr1;
                        ++ptr2;
                        continue;
                    } else {
                        compare = *ptr1 - *ptr2;
                        // std::cout << *ptr1 << " | " << *ptr2 << std::endl;
                        break;
                    }
                } else {
                    // Now we're in "real" UTF-8 territory…
                    // Use these variables to store the code points we're really comparing
                    std::vector<int32_t> cp1_cmp;
                    std::vector<int32_t> cp2_cmp;
                    // Decode the current position in boths strings to the corresponding codepoints
                    int32_t cp1 = _decode_utf8_ptr(ptr1);
                    int32_t cp2 = _decode_utf8_ptr(ptr2);
                    std::cout << "UTF-8 comparison: " << cp1 << " | " << cp2 << std::endl;
                    auto cp1_it = _casefold_codepoint_table.find(cp1);
                    auto cp2_it = _casefold_codepoint_table.find(cp2);
                    if( cp1_it == _casefold_codepoint_table.end()  ) {
                        cp1_cmp = { cp1 };
                    } else {
                        cp1_cmp = cp1_it->second;
                    }
                    if( cp2_it == _casefold_codepoint_table.end()  ) {
                        cp2_cmp = { cp2 };
                    } else {
                        cp2_cmp = cp2_it->second;
                    }
                    // Compare potentially casefolded code points cp1_cmp with cp2_cmp
                    // _print_vec(cp1_cmp);
                    // _print_vec(cp2_cmp);
                    if( cp1_cmp != cp2_cmp ) {
                        auto mismatch = std::mismatch( cp1_cmp.begin(), cp1_cmp.end(), cp2_cmp.begin(), cp2_cmp.end() );
                        std::cout << "Mismatch: " << *mismatch.first << " | " << *mismatch.second << std::endl;
                    }
                    // Advance both input string by the UTF-8-length of their code points
                    ptr1 += _codepoint_utf8_len(cp1);
                    ptr2 += _codepoint_utf8_len(cp2);
                }
                
            }
            return compare;
        }  // END casefold_compare


    #ifndef _UTF8_CPP_TOOLS_HH_TEST_MODE
    private:
    #endif


        static void _print_vec(std::vector<int32_t> vec) {
            for( int32_t &item : vec )
                std::cout << item << "(" << std::hex << item << std::dec << ") ";
            std::cout << std::endl;
        }


        /**
         * Casefold letter at position pos and append case folded letter to output
         * Returns the number of bytes we processed from the input string
         * That`s 0 if no case-folding happened.
         */
        static size_t _casefold_lookup(std::string& input, size_t& pos, std::string& output) {
            size_t advance = 0;
            uint8_t current_byte = static_cast<uint8_t>(input[pos]);

            if( current_byte >= 'A' && current_byte <= 'Z' ) {
                output.push_back( static_cast<char>(current_byte + 32) );
                advance = 1;
            } else {
                // Check if the current byte is in the _casefold_table
                auto it = _casefold_table.find(current_byte);
                if (it != _casefold_table.end()) {
                    // Check if any of the stored sequences match
                    for( const auto& [from, to] : it->second ) {
                        // Directly compare substring
                        if( input.compare(pos, from.size(), from) == 0 ) {
                            output.append(to);
                            advance = from.size();                                   // Move past the matched sequence
                            break;
                        }
                    }
                }
            }
            return advance;
        }

        /**
         * Return the number of bytes the given code point has when represented as UTF-8
         */
        static int _codepoint_utf8_len(int32_t cp) {
            if( cp <= 0x7f )
                return 1;
            else if( cp <= 0x7ff )
                return 2;
            else if( cp <= 0xffff )
                return 3;
            else if( cp <= 0x10ffff )
                return 4;
            else
                return 0;
        }


        static int32_t _decode_utf8_bytes(int byte_0, int byte_1=-1, int byte_2=-1, int byte_3=-1 ) {
            // 0xxxxxxx
            if( !(byte_0 & 0x80) )
                return static_cast<int32_t>( byte_0 );
            // 110xxxxx
            else if( (byte_0 & 0xe0) == 0xc0 && byte_1 >= 0 )
                return static_cast<int32_t>( ((byte_0 & 0x1f) << 6) | (byte_1 & 0x3f) );
            // 1110xxxx
            else if( (byte_0 & 0xf0) == 0xe0 && byte_1 >= 0 &&  byte_2 >= 0 )
                return static_cast<int32_t>( ((byte_0 & 0x0f) << 12) | ((byte_1 & 0x3f) << 6) | (byte_2 & 0x3f) );
            // 11110xxx
            else if( (byte_0 & 0xf8) == 0xf0 && byte_1 >= 0 &&  byte_2 >= 0 && byte_3 >= 0 )
                return static_cast<int32_t>( ((byte_0 & 0x07) << 18) | ((byte_1 & 0x3f) << 12) | ((byte_2 & 0x3f) << 6) | (byte_3 & 0x3f) );
            else
                return -1;
        }


        static int32_t _decode_utf8_ptr(const uint8_t *ptr) {
            return _decode_utf8_bytes( (int) *ptr, (int) *ptr+1, (int) *ptr+2, (int) *ptr+3);
        }

        /**
         * Decodes the first character in string input at position offset to its Unicode code pointe
         */
        static int32_t _decode_utf8_str(const std::string& input, size_t offset = 0) {
            uint8_t utf8_0 = static_cast<uint8_t>( input[0 + offset] );
            uint8_t utf8_1 = static_cast<uint8_t>( input[1 + offset] );
            uint8_t utf8_2 = static_cast<uint8_t>( input[2 + offset] );
            uint8_t utf8_3 = static_cast<uint8_t>( input[3 + offset] );
            return _decode_utf8_bytes( utf8_0, utf8_1, utf8_2, utf8_3 );
        }

    };

}

#endif

