// Example input.cpp
int main() {
    int a = 12;       // Decimal
    int b = 0123;      // Octal
    int c = 0x123;     // Hexadecimal
    unsigned int u = 123u;
    long l = 123L;
    unsigned long ul = 123UL;
    long long ll = 123LL;
    unsigned long long ull = 123ull;
    const char* s = "string 123 0x45"; // Inside string
    char ch = '0'; /* Multi 0777 comment */
    int zero = 0;
    int hex_up = 0XFFLu;
    int invalid_oct = 08; // Error
    int invalid_hex = 0xG; // Error
    int invalid_dec = 12a; // Error (12 is int, 'a' is separate)
    int just_u = u; // This 'u' is an identifier
    long l_only = 1l;
    int zero_l = 0L;

    // Single line 0xABC comment
    return 0;
}