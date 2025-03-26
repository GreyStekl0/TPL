/* Test cases for Lab 2 */
int main() {
    int a = 123;
    int b = 0xAb; // Hex
    int c = 017;  // Octal
    int d = 09; // Invalid octal ERROR
    float pi = 3.14159;
    double big = 1.23e10;
    double small = .5e-2;
    char x = 'a'; // Char
    const char* msg = "string /* not a comment */";
    a = a + /* multi
             line */ c;
    b = 0XFF;
    // Single line comment 123.45
    // Edge cases:
    // a = ..5; // Should be dot, dot, int(5)
    // b = 1.e; // ERROR
    // c = 0x; // ERROR
    // d = 10e+; // ERROR
    return 0;
}