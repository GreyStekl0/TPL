#include <iostream>
#include <fstream>
#include <cstdio>

enum State { NORMAL, IN_COMMENT, IN_STRING, IN_CHAR };

int main() {
    const char* fileName = "lab01.example.utf8.c";
    const char* tempFileName = "temp_output.c";

    std::ifstream in(fileName);
    if (!in) {
        std::cerr << "Не удалось открыть файл " << fileName << std::endl;
        return 1;
    }

    std::ofstream out(tempFileName);

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

    std::rename(tempFileName, fileName);

    return 0;
}