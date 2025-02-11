#include <iostream>
#include <fstream>

enum State { NORMAL, IN_COMMENT, IN_STRING, IN_CHAR };

int main() {
    std::ifstream in("input.cpp");
    if (!in) {
        std::cerr << "Не удалось открыть файл input.cpp" << std::endl;
        return 1;
    }

    std::ofstream out("output.cpp");

    State state = NORMAL;
    char c;

    while (in.get(c)) {
        switch(state) {
            case NORMAL:
                if (c == '/') {
                    if (in.peek() == '*') {
                        in.get(c);
                        state = IN_COMMENT;
                    } else {
                        out.put(c);
                    }
                } else if (c == '"') {
                    out.put(c);
                    state = IN_STRING;
                } else if (c == '\'') {
                    out.put(c);
                    state = IN_CHAR;
                } else {
                    out.put(c);
                }
                break;

            case IN_COMMENT:
                if (c == '*') {
                    if (in.peek() == '/') {
                        in.get(c);
                        state = NORMAL;
                    }
                }
                break;

            case IN_STRING:
                out.put(c);
                if (c == '\\') {
                    if (in.get(c)) {
                        out.put(c);
                    }
                } else if (c == '"') {
                    state = NORMAL;
                }
                break;

            case IN_CHAR:
                out.put(c);
                if (c == '\\') {
                    if (in.get(c)) {
                        out.put(c);
                    }
                } else if (c == '\'') {
                    state = NORMAL;
                }
                break;
        }
    }

    in.close();
    out.close();

    return 0;
}
