#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype> // For isdigit, isxdigit, tolower

// Enum defining the states of the Finite State Machine
enum State
{
    NORMAL,
    SLASH,
    MULTI_COMMENT,
    STAR_IN_MULTI_COMMENT,
    SINGLE_COMMENT,
    IN_STRING,
    IN_CHAR,
    SLASH_IN_STRING,
    SLASH_IN_CHAR,

    // --- States for Number Recognition ---
    ZERO_START, // Started with '0'
    OCTAL, // Parsing an octal number (after '0')
    HEX_START, // Seen '0x' or '0X'
    HEX, // Parsing a hexadecimal number
    DECIMAL_INTEGER, // Parsing a decimal integer (started with 1-9)
    AFTER_DOT, // Seen a '.' (potentially start of float or part of ..)
    FRACTION, // Parsing fractional part (after '.')
    EXPONENT_START, // Seen 'e' or 'E'
    EXPONENT_SIGN, // Seen 'e' or 'E' followed by '+' or '-'
    EXPONENT, // Parsing exponent digits
    // SUFFIX state might be needed for full compliance, but adds complexity
    INVALID_NUMBER // Encountered an invalid sequence during number parsing
};

// Helper to check for octal digits
bool isoctdigit(char c)
{
    return c >= '0' && c <= '7';
}

// Function to determine and print the number type
// Returns true if the sequence was potentially valid (even if ending in suffix), false if clearly invalid.
// IMPORTANT: Adds a space after the output token.
bool process_number(const std::string& num_str, State current_fsm_state, std::ofstream& out)
{
    if (num_str.empty()) return true;

    // Determine context based on the state the FSM is IN when this is called
    // This helps distinguish things like "0" (decimal int) vs intermediate states
    State effective_end_state = current_fsm_state;
    if (current_fsm_state == INVALID_NUMBER)
    {
        out << "ERROR(" << num_str << ") ";
        return false;
    }


    // --- Type Determination Logic ---
    bool is_float = false;
    bool is_hex = false;
    bool is_octal = false;
    // Suffix flags would be set here in a more complex suffix parser
    // bool suffix_u = false;
    // int  suffix_l = 0;
    // bool suffix_f = false;

    // Check content for float indicators
    for (char c : num_str)
    {
        if (c == '.' || c == 'e' || c == 'E')
        {
            is_float = true;
            break;
        }
    }

    // Check for hex/octal prefix (simplified)
    if (num_str.length() > 1 && num_str[0] == '0')
    {
        if (std::tolower(num_str[1]) == 'x')
        {
            is_hex = true;
            is_float = false; // Hex cannot be float directly (0x1.2p1 is C99 hex float, not handled)
        }
        else if (isdigit(num_str[1]) && !is_float)
        {
            // Octal if not already float
            is_octal = true;
            for (size_t i = 1; i < num_str.length(); ++i)
            {
                if (!isoctdigit(num_str[i]))
                {
                    // Check for invalid octal digits like 8 or 9
                    out << "ERROR(" << num_str << ") ";
                    return false;
                }
            }
        }
        // Otherwise, it might be a float starting 0. or 0e...
    }

    // --- Output based on type ---
    // This is simplified; real C++ has complex rules for default types and suffixes.
    if (is_float)
    {
        // Check for required digits (e.g., after '.', 'e', '+/-')
        char last = num_str.back();
        if (last == '.' || last == 'e' || last == 'E' || last == '+' || last == '-')
        {
            out << "ERROR(" << num_str << ") "; // Incomplete float
            return false;
        }
        // Simplification: Assume 'double' unless suffix indicates otherwise
        out << "double(" << num_str << ") ";
    }
    else if (is_hex)
    {
        if (num_str.length() <= 2)
        {
            // "0x" is invalid
            out << "ERROR(" << num_str << ") ";
            return false;
        }
        // Simplification: Assume 'int' unless suffix indicates otherwise
        out << "int_hex(" << num_str << ") ";
    }
    else if (is_octal)
    {
        // Simplification: Assume 'int' unless suffix indicates otherwise
        out << "int_oct(" << num_str << ") ";
    }
    else
    {
        // Decimal integer (includes "0")
        // Simplification: Assume 'int' unless suffix indicates otherwise
        out << "int(" << num_str << ") ";
    }

    return true; // Sequence was processed as potentially valid
}


int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in)
    {
        std::cerr << "Could not open input file: " << argv[1] << std::endl;
        return 1;
    }

    std::ofstream out(argv[2], std::ios::binary);
    if (!out)
    {
        std::cerr << "Could not open output file: " << argv[2] << std::endl;
        return 1;
    }

    State state = NORMAL;
    char c;
    std::string current_number;
    // Removed processed_char flag and last_numeric_state (use current state for context)

    while (in.get(c))
    {
        switch (state)
        {
        case NORMAL:
            if (c == '/')
            {
                state = SLASH;
            }
            else if (c == '"')
            {
                out.put(c);
                state = IN_STRING;
            }
            else if (c == '\'')
            {
                out.put(c);
                state = IN_CHAR;
            }
            else if (isdigit(c))
            {
                current_number += c;
                state = (c == '0') ? ZERO_START : DECIMAL_INTEGER;
            }
            else if (c == '.')
            {
                current_number += c;
                state = AFTER_DOT; // Potential start of float like .5
            }
            else
            {
                out.put(c); // Normal character
            }
            break;

        // --- Comment/String/Char states (unchanged from Lab 1) ---
        case SLASH:
            if (c == '*') { state = MULTI_COMMENT; }
            else if (c == '/') { state = SINGLE_COMMENT; }
            else
            {
                out.put('/'); // Output the buffered slash
                state = NORMAL;
                in.putback(c); // Put back the char that followed '/'
            }
            break;
        case MULTI_COMMENT:
            if (c == '*') { state = STAR_IN_MULTI_COMMENT; }
            break;
        case STAR_IN_MULTI_COMMENT:
            if (c == '/')
            {
                out.put(' ');
                state = NORMAL;
            } // Replace comment with space
            else if (c != '*') { state = MULTI_COMMENT; }
        // else stay in STAR_IN_MULTI_COMMENT if c == '*'
            break;
        case SINGLE_COMMENT:
            if (c == '\n' || c == '\r')
            {
                out.put(c);
                state = NORMAL;
            } // Keep newline
        // else stay in SINGLE_COMMENT (ignore char)
            break;
        case IN_STRING:
            out.put(c);
            if (c == '\\') { state = SLASH_IN_STRING; }
            else if (c == '"') { state = NORMAL; }
            break;
        case IN_CHAR:
            out.put(c);
            if (c == '\\') { state = SLASH_IN_CHAR; }
            else if (c == '\'') { state = NORMAL; }
            break;
        case SLASH_IN_STRING:
            out.put(c);
            state = IN_STRING;
            break;
        case SLASH_IN_CHAR:
            out.put(c);
            state = IN_CHAR;
            break;

        // --- Number Recognition States (using putback) ---
        case ZERO_START: // After '0'
            if (std::tolower(c) == 'x')
            {
                current_number += c;
                state = HEX_START;
            }
            else if (isoctdigit(c))
            {
                current_number += c;
                state = OCTAL;
            }
            else if (c == '.')
            {
                current_number += c;
                state = FRACTION; // e.g., 0.1
            }
            else if (std::tolower(c) == 'e')
            {
                current_number += c;
                state = EXPONENT_START; // e.g., 0e1
            }
            else if (isdigit(c))
            {
                // Invalid octal like 08, 09
                current_number += c;
                state = INVALID_NUMBER; // Consume the invalid digit
            }
            // Add suffix checks here if needed (L, U, F...)
            else
            {
                // End of number '0' or '0.' or '0e' etc.
                process_number(current_number, state, out); // Process what we have
                current_number.clear();
                state = NORMAL;
                in.putback(c); // Put back the terminating char
            }
            break;

        case OCTAL: // Parsing octal digits
            if (isoctdigit(c))
            {
                current_number += c;
            }
            else if (isdigit(c) || std::tolower(c) == 'x' || c == '.')
            {
                // Invalid octal digit or construct
                current_number += c;
                state = INVALID_NUMBER; // Consume invalid char
            }
            // Add suffix checks here (L, U...)
            else
            {
                // End of octal number
                process_number(current_number, state, out);
                current_number.clear();
                state = NORMAL;
                in.putback(c);
            }
            break;

        case HEX_START: // After '0x' or '0X'
            if (isxdigit(c))
            {
                current_number += c;
                state = HEX;
            }
            else
            {
                // 0x followed by non-hex is invalid
                // Don't consume 'c', treat '0x' as error
                process_number(current_number, state, out); // Will likely be marked error by process_number
                current_number.clear();
                state = NORMAL;
                in.putback(c); // Put back the non-hex char
            }
            break;

        case HEX: // Parsing hex digits
            if (isxdigit(c))
            {
                current_number += c;
            }
            // Add suffix checks here (L, U...)
            else
            {
                // End of hex number
                process_number(current_number, state, out);
                current_number.clear();
                state = NORMAL;
                in.putback(c);
            }
            break;

        case DECIMAL_INTEGER: // Started with 1-9
            if (isdigit(c))
            {
                current_number += c;
            }
            else if (c == '.')
            {
                current_number += c;
                state = FRACTION;
            }
            else if (std::tolower(c) == 'e')
            {
                current_number += c;
                state = EXPONENT_START;
            }
            // Add suffix checks here (L, U, F...)
            else
            {
                // End of decimal integer
                process_number(current_number, state, out);
                current_number.clear();
                state = NORMAL;
                in.putback(c);
            }
            break;

        case AFTER_DOT: // Started with '.'
            if (isdigit(c))
            {
                current_number += c;
                state = FRACTION; // e.g., .5
            }
            else
            {
                // '.' followed by non-digit is not a number start
                out << "."; // Output the dot that was buffered
                current_number.clear();
                state = NORMAL;
                in.putback(c); // Put back the non-digit character
            }
            break;

        case FRACTION: // After '.' and at least one digit, or after integer and '.'
            if (isdigit(c))
            {
                current_number += c;
            }
            else if (std::tolower(c) == 'e')
            {
                current_number += c;
                state = EXPONENT_START;
            }
            // Add suffix checks here (F, L...)
            else
            {
                // End of fractional part
                process_number(current_number, state, out);
                current_number.clear();
                state = NORMAL;
                in.putback(c);
            }
            break;

        case EXPONENT_START: // After 'e' or 'E'
            if (isdigit(c))
            {
                current_number += c;
                state = EXPONENT;
            }
            else if (c == '+' || c == '-')
            {
                current_number += c;
                state = EXPONENT_SIGN;
            }
            else
            {
                // 'e'/'E' not followed by digit or sign is error
                process_number(current_number, state, out); // Mark as error
                current_number.clear();
                state = NORMAL;
                in.putback(c); // Put back the problematic char
            }
            break;

        case EXPONENT_SIGN: // After 'e'/'E' and '+' or '-'
            if (isdigit(c))
            {
                current_number += c;
                state = EXPONENT;
            }
            else
            {
                // Sign not followed by digit is error
                process_number(current_number, state, out); // Mark as error
                current_number.clear();
                state = NORMAL;
                in.putback(c); // Put back the problematic char
            }
            break;

        case EXPONENT: // Parsing exponent digits
            if (isdigit(c))
            {
                current_number += c;
            }
            // Add suffix checks here (F, L...)
            else
            {
                // End of exponent part
                process_number(current_number, state, out);
                current_number.clear();
                state = NORMAL;
                in.putback(c);
            }
            break;

        case INVALID_NUMBER: // Consuming until end of invalid number sequence
            // Keep consuming potential parts of the bad number
            if (isdigit(c) || isalpha(c) || c == '.')
            {
                current_number += c;
            }
            else
            {
                // End of invalid sequence
                process_number(current_number, state, out); // Report the error
                current_number.clear();
                state = NORMAL;
                in.putback(c); // Put back the terminating character
            }
            break;
        } // end switch(state)
    } // end while(in.get(c))

    // --- Handle End of File ---
    // If EOF is reached while processing a number, finalize it.
    if (!current_number.empty())
    {
        // Use 'state' as context for process_number
        if (state == AFTER_DOT)
        {
            // Special case: '.' at EOF is just a dot.
            out << ".";
        }
        // Let process_number handle final validation based on the state we were in
        else
        {
            process_number(current_number, state, out);
        }
    }
    else if (state == SLASH)
    {
        // Handle '/' at EOF
        out.put('/');
    }

    in.close();
    out.close();

    std::cout << "Processing complete. Output written to " << argv[2] << std::endl;

    return 0;
}
