#include <iostream>
#include <fstream>

enum State {
    NORMAL,         // обычное состояние – выводим символы в выходной поток
    SLASH,          // только что прочитали '/', ожидаем следующий символ для определения комментария
    COMMENT,        // внутри многострочного комментария
    STAR,           // после '*' внутри многострочного комментария – проверяем, не конец ли комментария
    SINGLE_COMMENT, // внутри однострочного комментария (начало с //)
    IN_STRING,      // внутри строкового литерала (начинается с ")
    IN_CHAR         // внутри символьной константы (начинается с ')
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Использование: " << argv[0] << " <входной файл> <выходной файл>\n";
        return 1;
    }

    std::ifstream fin(argv[1], std::ios::in);
    if (!fin) {
        std::cerr << "Ошибка открытия входного файла: " << argv[1] << "\n";
        return 1;
    }

    std::ofstream fout(argv[2], std::ios::out);
    if (!fout) {
        std::cerr << "Ошибка открытия выходного файла: " << argv[2] << "\n";
        fin.close();
        return 1;
    }

    State state = NORMAL;
    char c;

    while (fin.get(c)) {
        switch (state) {
            case NORMAL:
                if (c == '/') {
                    // При обнаружении '/' переходим в состояние SLASH, чтобы проверить, начинается ли комментарий
                    state = SLASH;
                } else if (c == '"') {
                    // Начало строкового литерала
                    fout.put(c);
                    state = IN_STRING;
                } else if (c == '\'') {
                    // Начало символьной константы
                    fout.put(c);
                    state = IN_CHAR;
                } else {
                    // Обычный символ – выводим его
                    fout.put(c);
                }
                break;

            case SLASH:
                if (c == '*') {
                    // Начинается многострочный комментарий
                    state = COMMENT;
                } else if (c == '/') {
                    // Начинается однострочный комментарий
                    state = SINGLE_COMMENT;
                } else {
                    // Это не комментарий: выводим ранее прочитанный '/' и текущий символ
                    fout.put('/');
                    fout.put(c);
                    state = NORMAL;
                }
                break;

            case COMMENT:
                if (c == '*') {
                    // Возможно, конец многострочного комментария – переходим в состояние STAR
                    state = STAR;
                }
                // Иначе символы игнорируются
                break;

            case STAR:
                if (c == '/') {
                    // Найден конец многострочного комментария, возвращаемся в NORMAL
                    state = NORMAL;
                } else if (c != '*') {
                    // Если встретили не '*', возвращаемся в состояние COMMENT
                    state = COMMENT;
                }
                // Если c == '*', остаёмся в состоянии STAR
                break;

            case SINGLE_COMMENT:
                // Пропускаем символы однострочного комментария до символа перевода строки
                if (c == '\n' || c == '\r') {
                    fout.put(c);
                    state = NORMAL;
                }
                break;

            case IN_STRING:
                // Выводим все символы внутри строкового литерала
                fout.put(c);
                if (c == '\\') {
                    // Экранирование – выводим следующий символ без проверки
                    if (fin.get(c)) {
                        fout.put(c);
                    }
                } else if (c == '"') {
                    // Завершение строкового литерала
                    state = NORMAL;
                }
                break;

            case IN_CHAR:
                // Выводим символы внутри символьной константы
                fout.put(c);
                if (c == '\\') {
                    // Экранирование – выводим следующий символ
                    if (fin.get(c)) {
                        fout.put(c);
                    }
                } else if (c == '\'') {
                    // Завершение символьной константы
                    state = NORMAL;
                }
                break;
        }
    }

    // Если файл закончился сразу после символа '/', выводим его
    if (state == SLASH) {
        fout.put('/');
    }

    fin.close();
    fout.close();

    return 0;
}
