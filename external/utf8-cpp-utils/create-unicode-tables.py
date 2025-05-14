"""
 * SPDX-License-Identifier: GPL-3.0-or-later
 * 
 * Copyright (C) 2025 Stefan Fischerländer
 * 
 * This file is part of Tablecruncher.
 * 
 * Tablecruncher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * Tablecruncher is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Tablecruncher. If not, see <https://www.gnu.org/licenses/>.
"""

import sys


# Generate a single header file containing everything
header_filename = sys.argv[1]



# we're storing the casefolded letters here, ordered by their first byte
casefold_table = {}
casefold_codepoint_table = {}

for cp in range(128,sys.maxunicode+1):
    char = chr(cp)
    cf_char = char.casefold()
    if char != cf_char:
        # only create table entry if this code point can be case folded
        char_bytes = char.encode('utf-8')
        cf_char_bytes = cf_char.encode('utf-8')
        # print( f"{char}\t{cf_char}\t{char_bytes}\t{cf_char_bytes}" )
        char_group = casefold_table.setdefault(char_bytes[0], [])
        char_group.append( (char_bytes, cf_char_bytes) )
        # create a codepoint table with casefoldings
        #print(f"{char} ({hex(cp)}) : {cf_char}")
        casefold_codepoint_table[cp] = [ord(x) for x in cf_char]



print(f"Generated header file…")


with open(header_filename, "w", encoding="utf-8") as f:
    f.write("#ifndef _UTF8_CPP_TABLE_HH\n#define _UTF8_CPP_TABLE_HH\n\n")
    f.write("#include <map>\n#include <vector>\n#include <string>\n#include <cstdint>\n\n")
    f.write("typedef std::pair<std::string, std::string> replacement_t;\n\n")

    f.write("namespace Utf8CppUtils {\n\n");
    f.write("inline const std::map<uint8_t, std::vector<replacement_t>> _casefold_table = {\n{\n")

    # Iterate through first-byte keys
    for first_byte, replacements in sorted(casefold_table.items()):
        f.write(f"    {{ 0x{first_byte:02x}, {{\n")

        for original, replacement in replacements:
            orig_str = ''.join(f'\\x{b:02x}' for b in original)
            repl_str = ''.join(f'\\x{b:02x}' for b in replacement)
            f.write(f'        {{ "{orig_str}", "{repl_str}" }},\n')

        f.write("    }},\n")  # Closing braces for vector and map entry

    f.write("}}; // END _casefold_table\n\n")

    #
    # Write the codepoint conversion table
    f.write("inline const std::map<int32_t, std::vector<int32_t>> _casefold_codepoint_table = {\n")
    for cp, cf_cp in sorted(casefold_codepoint_table.items()):
        cp_list = ', '.join([hex(x) for x in cf_cp])
        f.write(f"    {{ {hex(cp)}, {{ {cp_list} }} }},\n")
    f.write("}; // END _casefold_codepoint_table\n\n")


    f.write("} // namespace Utf8CppUtils\n\n");
    f.write("#endif // _UTF8_CPP_TABLE_HH\n")

print(f"Generated {header_filename}.")