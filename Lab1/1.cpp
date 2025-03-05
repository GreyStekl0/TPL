#include <iostream>
#include <fstream>

enum State {
    NORMAL,
    SLASH,
    MULTI_COMMENT,
    STAR_IN_COMMENT
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Could not open input file." << std::endl;
        return 1;
    }

    std::ofstream out(argv[2], std::ios::binary);
    if (!out) {
        std::cerr << "Could not open output file." << std::endl;
        return 1;
    }

    State state = NORMAL;
    char c;
    while (in.get(c)) {
        switch (state) {
            case NORMAL:
                if (c == '/') {
                    state = SLASH;
                } else {
                    out.put(c);
                }
                break;
            case SLASH:
                if (c == '*') {
                    state = MULTI_COMMENT;
                } else if (c == '/') {
                    out.put('/');
                } else {
                    out.put('/');
                    out.put(c);
                    state = NORMAL;
                }
                break;
            case MULTI_COMMENT:
                if (c == '*') {
                    state = STAR_IN_COMMENT;
                }
                break;
            case STAR_IN_COMMENT:
                if (c == '/') {
                    state = NORMAL;
                } else if (c != '*') {
                    state = MULTI_COMMENT;
                }
                break;
        }
    }
    if (state == SLASH) {
        out.put('/');
    }

    in.close();
    out.close();
    return 0;
}
