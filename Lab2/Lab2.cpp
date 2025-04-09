#include <iostream>
#include <fstream>
#include <string>
#include <cctype>    // Для isdigit, isxdigit, tolower, isalpha, isalnum, isspace

// Состояния внешнего автомата (комментарии, строки)
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
    SLASH_IN_CHAR
};

// Состояния внутреннего автомата для распознавания чисел
enum NumberState
{
    IDLE,             // Начальное состояние (не число)
    START_ZERO,       // Встретили '0'
    OCTAL,            // Внутри восьмеричного числа (после '0')
    DECIMAL,          // Внутри десятичного числа
    HEX_START,        // Встретили '0x' или '0X'
    HEX,              // Внутри шестнадцатеричного числа
    NUMBER_END_POTENTIAL_SUFFIX, // Числовая часть закончилась, след. символ - буква
    SUFFIX_U,         // Встретили 'u' в суффиксе
    SUFFIX_L,         // Встретили 'l' в суффиксе
    SUFFIX_LL,        // Встретили 'll' в суффиксе
    SUFFIX_UL,        // Встретили 'ul' или 'lu'
    SUFFIX_ULL,       // Встретили 'ull' или 'llu'
    INVALID           // Недопустимая последовательность
};

// Функция для определения типа константы по суффиксам
// (l_count: 0 для нет 'l', 1 для 'l', 2 для 'll')
std::string get_int_type(bool has_u, int l_count)
{
    if (has_u)
    {
        if (l_count == 0) return "unsigned int";
        if (l_count == 1) return "unsigned long";
        return "unsigned long long"; // l_count >= 2
    }
    else
    {
        if (l_count == 0) return "int"; // Тип по умолчанию
        if (l_count == 1) return "long";
        return "long long"; // l_count >= 2
    }
}

// Функция для проверки, является ли символ разделителем (завершает токен)
// Добавим точку, т.к. 123. - это ошибка для int
bool is_delimiter(char c) {
    return std::isspace(c) || std::string("+-*/%=(){}[];,<>&|^!~.").find(c) != std::string::npos;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input file> <report file>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in)
    {
        std::cerr << "Could not open input file." << std::endl;
        return 1;
    }

    std::ofstream report_out(argv[2]);

    State state = NORMAL;
    NumberState num_state = IDLE;
    std::string current_token;
    char c;
    // Символ, сохраненный для проверки на принадлежность к суффиксу
    char potential_suffix_char = 0;

    // --- Лямбда-функция для финализации токена ---
    // Вызывается ПЕРЕД обработкой символа, который ЗАВЕРШАЕТ токен
        // --- Лямбда-функция для финализации токена ---
    // Вызывается ПЕРЕД обработкой символа, который ЗАВЕРШАЕТ токен
    auto finalize_token = [&]() {
        // --- Начальные проверки ---
        if (current_token.empty()) { // Если токена нет, ничего не делаем
             // Убедимся, что состояние сброшено на всякий случай
             num_state = IDLE;
             potential_suffix_char = 0;
             return;
        }
        // Если состояние уже IDLE, но токен не пуст (не должно происходить при норм. работе)
        if (num_state == IDLE) {
             // Просто игнорируем "осиротевший" токен
             current_token = "";
             potential_suffix_char = 0;
             return;
        }

        // --- Основная логика анализа ---

        // 1. Если автомат уже в состоянии INVALID, это точно ошибка
        if (num_state == INVALID) {
             report_out << current_token << "\tERROR" << std::endl;
        } else {
            // 2. Автомат НЕ в состоянии INVALID. Анализируем токен.
            size_t number_end_pos = 0; // Индекс *после* последнего символа числа
            bool parse_error = false;   // Флаг ошибки парсинга базы
            bool is_hex = (current_token.length() > 1 && current_token[0] == '0' && (tolower(current_token[1]) == 'x'));
            // Считаем восьмеричным кандидатом, только если есть цифры после '0'
            bool is_octal_candidate = (current_token.length() > 1 && current_token[0] == '0' && !is_hex);
             // Ноль сам по себе - десятичный (или int по типу)
             if (current_token == "0") is_octal_candidate = false;

            // --- Определяем конец числовой части и проверяем ошибки базы ---
             if (is_hex) {
                 // Минимум "0x" + 1 цифра
                 if (current_token.length() <= 2) {
                      parse_error = true;
                      number_end_pos = current_token.length(); // Весь токен - ошибка
                 } else {
                     number_end_pos = 2;
                     while (number_end_pos < current_token.length() && std::isxdigit(current_token[number_end_pos])) {
                         number_end_pos++;
                     }
                     // Если после 0x не было ВООБЩЕ цифр (хотя while бы не сработал)
                     if (number_end_pos == 2) parse_error = true;
                 }
            } else { // Десятичное или восьмеричное
                 number_end_pos = 0;
                 while (number_end_pos < current_token.length() && std::isdigit(current_token[number_end_pos])) {
                     // Проверяем на 8 или 9 только если это восьмеричный кандидат
                     if (is_octal_candidate && current_token[number_end_pos] > '7') {
                         parse_error = true; // Нашли 8 или 9 в восьмеричном
                         // Не прерываем цикл, чтобы захватить весь ошибочный токен
                     }
                     number_end_pos++;
                 }
                 // Если не было найдено ни одной цифры (например, токен "+")
                 if (number_end_pos == 0 && current_token != "0") {
                     // current_token == "0" обрабатывается отдельно
                     parse_error = true;
                 }
                 // "0" сам по себе валиден
                 if (current_token == "0") parse_error = false;
            }

            // --- Извлекаем и проверяем суффикс ---
            std::string suffix = "";
            bool has_u = false;
            int l_count = 0; // 0 = нет, 1 = l, 2 = ll
            bool suffix_is_valid = true; // Предполагаем валидность, если суффикса нет

            // Проверяем, есть ли символы ПОСЛЕ определенной числовой части
            if (number_end_pos < current_token.length()) {
                // Есть символы - это суффикс
                suffix = current_token.substr(number_end_pos);
                size_t u_count_actual = 0;
                size_t l_count_actual = 0;

                // Проверяем КАЖДЫЙ символ суффикса
                for (char sc : suffix) {
                    char lsc = tolower(sc);
                    if (lsc == 'u') u_count_actual++;
                    else if (lsc == 'l') l_count_actual++;
                    else {
                        suffix_is_valid = false; // Недопустимый символ в суффиксе
                        break; // Дальше можно не проверять
                    }
                }

                // Если символы были допустимы, проверяем их КОЛИЧЕСТВО
                if (suffix_is_valid) {
                    if (u_count_actual > 1 || l_count_actual > 2) {
                        suffix_is_valid = false; // Слишком много 'u' или 'l'
                    } else {
                        // Устанавливаем флаги для get_int_type
                        has_u = (u_count_actual == 1);
                        if (l_count_actual == 1) l_count = 1;
                        else if (l_count_actual == 2) l_count = 2;
                        else l_count = 0;
                    }
                }
            } // else: Суффикса нет, suffix_is_valid остается true

            // --- Проверяем конечное состояние автомата ---
            // Допустимые КОНЕЧНЫЕ состояния (где токен может закончиться)
             bool valid_final_num_state = (
                 num_state == START_ZERO || num_state == DECIMAL || num_state == OCTAL || num_state == HEX ||
                 num_state == SUFFIX_U || num_state == SUFFIX_L || num_state == SUFFIX_LL ||
                 num_state == SUFFIX_UL || num_state == SUFFIX_ULL
             );
             // Состояние NUMBER_END_POTENTIAL_SUFFIX означает, что после числа
             // была буква, которая НЕ начала валидный суффикс - это ошибка.
             if (num_state == NUMBER_END_POTENTIAL_SUFFIX) {
                  valid_final_num_state = false; // Не может быть конечным
                  // parse_error = true; // Можно пометить как ошибку парсинга
                  // Хотя parse_error относится к базе, а это ошибка структуры
             }


            // --- Итоговое решение ---
            if (parse_error || !suffix_is_valid || !valid_final_num_state) {
                // Если была ошибка парсинга базы, ИЛИ невалидный суффикс,
                // ИЛИ автомат остановился в недопустимом конечном состоянии
                 report_out << current_token << "\tERROR" << std::endl;
            } else {
                 // Ошибок нет, состояние финальное валидное
                 report_out << current_token << "\t" << get_int_type(has_u, l_count) << std::endl;
            }
        } // Конец блока else (если состояние НЕ INVALID)

        // --- Сброс состояния и токена в конце финализации ---
        current_token = "";
        num_state = IDLE;
        potential_suffix_char = 0;
    };
    // --- Конец лямбда-функции finalize_token ---

    // --- Основной цикл чтения файла ---
    while (in.get(c))
    {
        // Вспомогательная переменная для повторной обработки символа
        char char_to_reprocess = 0;

    process_char_again: // Метка для повторной обработки

        // Если есть символ для повторной обработки, используем его
        if (char_to_reprocess != 0) {
            c = char_to_reprocess;
            char_to_reprocess = 0; // Сбрасываем
        }

        // 0. Обработка смены основного состояния (комментарии, строки)
        if (state == NORMAL) {
            // Проверяем, не начинается ли комментарий или строка
             if (c == '/') {
                 // Может быть комментарий ИЛИ завершение числа перед оператором
                 finalize_token(); // Завершаем предыдущий токен (если был)
                 state = SLASH;    // Переходим в состояние проверки '/'
                 continue;         // Переходим к след. итерации для обработки '/' в SLASH
             } else if (c == '"') {
                 finalize_token();
                 state = IN_STRING;
                 continue;
             } else if (c == '\'') {
                 finalize_token();
                 state = IN_CHAR;
                 continue;
             }
             // Если это не начало комм/строки, остаемся в NORMAL
        } else {
             // Мы НЕ в NORMAL, обрабатываем комментарии/строки и т.д.
             // (Логика как в Лаб 1, но без вывода символов)
              switch (state) {
                 case SLASH:
                     if (c == '*') state = MULTI_COMMENT;
                     else if (c == '/') state = SINGLE_COMMENT;
                     else { // Был оператор деления
                         state = NORMAL;
                         // Оператор '/' был проигнорирован, теперь обрабатываем 'c' в NORMAL
                         char_to_reprocess = c; // Повторно обработаем 'c'
                         goto process_char_again;
                     }
                     break;
                 // ... остальные case для комментариев/строк ...
                 case MULTI_COMMENT: if (c == '*') state = STAR_IN_MULTI_COMMENT; break;
                 case STAR_IN_MULTI_COMMENT: if (c == '/') state = NORMAL; else if (c != '*') state = MULTI_COMMENT; break;
                 case SINGLE_COMMENT: if (c == '\n' || c == '\r') state = NORMAL; break;
                 case IN_STRING: if (c == '\\') state = SLASH_IN_STRING; else if (c == '"') state = NORMAL; break;
                 case IN_CHAR: if (c == '\\') state = SLASH_IN_CHAR; else if (c == '\'') state = NORMAL; break;
                 case SLASH_IN_STRING: state = IN_STRING; break;
                 case SLASH_IN_CHAR: state = IN_CHAR; break;
                 default: state = NORMAL; break; // На всякий случай
             }
             continue; // Пропускаем обработку числа для не-NORMAL состояний
        }

        // --- Обработка символа 'c' в состоянии NORMAL ---

        // 1. Обработка состояния INVALID
        if (num_state == INVALID) {
             if (is_delimiter(c)) { // Разделитель завершает ошибочный токен
                 finalize_token(); // Выведет ошибку и сбросит state в IDLE
                 // Разделитель сам по себе игнорируется
             } else { // Считаем символ продолжением ошибочного токена
                 current_token += c;
             }
             continue; // Переходим к следующей итерации
        }

        // 2. Проверка на завершение ВАЛИДНОГО токена разделителем
        // (Исключаем NUMBER_END_POTENTIAL_SUFFIX, т.к. там символ - буква)
        if (num_state != IDLE && num_state != NUMBER_END_POTENTIAL_SUFFIX && is_delimiter(c)) {
            finalize_token(); // Завершаем валидный токен, выведет результат
            // Разделитель игнорируется
            continue; // Переходим к следующей итерации
        }

        // 3. Основная логика переходов автомата чисел
        bool expected_digit = false;
        switch (num_state)
        {
            case IDLE:
                if (c == '0') { num_state = START_ZERO; current_token += c; }
                else if (std::isdigit(c)) { num_state = DECIMAL; current_token += c; }
                // Иначе (буква, оператор и т.д.) - игнорируем, остаемся в IDLE
                break;

            case START_ZERO: // Мы прочитали '0'
                if (c == 'x' || c == 'X') { num_state = HEX_START; current_token += c; }
                else if (c >= '0' && c <= '7') { num_state = OCTAL; current_token += c; }
                else if (std::isalpha(c)) { // Возможно, суффикс
                    num_state = NUMBER_END_POTENTIAL_SUFFIX;
                    potential_suffix_char = c; // Сохраняем букву
                } else { // Недопустимый символ (8, 9, .) или разделитель
                    // Разделитель должен был вызвать finalize выше
                    current_token += c; num_state = INVALID;
                }
                break;

            case DECIMAL: // Внутри 1..9...
            case OCTAL:   // Внутри 0[0-7]...
            case HEX:     // Внутри 0x[0-f]...
                if (num_state == DECIMAL) expected_digit = std::isdigit(c);
                else if (num_state == OCTAL) expected_digit = (c >= '0' && c <= '7');
                else /* num_state == HEX */ expected_digit = std::isxdigit(c);

                if (expected_digit) {
                    current_token += c; // Остаемся в том же состоянии
                     if (num_state == OCTAL && c > '7') num_state = INVALID; // Явная ошибка для 8,9
                } else if (std::isalpha(c)) { // Возможно, суффикс
                    num_state = NUMBER_END_POTENTIAL_SUFFIX;
                    potential_suffix_char = c; // Сохраняем букву
                } else { // Не цифра нужной базы, не буква, не разделитель
                    // Разделитель должен был вызвать finalize выше
                    current_token += c; num_state = INVALID;
                }
                break;

             case HEX_START: // Мы прочитали '0x'
                if (std::isxdigit(c)) { num_state = HEX; current_token += c; }
                else { // Сразу после '0x' не hex-цифра
                    current_token += c; num_state = INVALID;
                }
                break;

            // ---> Обработка буквы после числа <---
            // ---> Обработка буквы после числа <---
        case NUMBER_END_POTENTIAL_SUFFIX:
            // 'potential_suffix_char' содержит букву, прочитанную на пред. шаге ('a')
                // 'c' - это символ, идущий ПОСЛЕ этой буквы (например, разделитель)
            {
                char first_letter = potential_suffix_char; // 'a'
                char first_letter_lower = tolower(first_letter);
                potential_suffix_char = 0; // Сбрасываем

                if (first_letter_lower == 'u') {
                    current_token += first_letter; // Добавляем 'u' к токену "12" -> "12u"
                    num_state = SUFFIX_U; // Переходим в состояние суффикса
                    char_to_reprocess = c; // Повторно обработаем 'c' в новом состоянии
                    goto process_char_again;
                } else if (first_letter_lower == 'l') {
                    current_token += first_letter; // Добавляем 'l' к токену "12" -> "12l"
                    num_state = SUFFIX_L;
                    char_to_reprocess = c;
                    goto process_char_again;
                } else {
                    // Буква была не 'u' и не 'l'. Это ошибка для целочисленной константы.
                    current_token += first_letter; // Добавляем 'a' к токену "12" -> "12a"
                    num_state = INVALID;           // Переходим в состояние ошибки
                    char_to_reprocess = c;         // Повторно обработаем 'c' в состоянии INVALID
                    // (скорее всего, 'c' будет разделителем и вызовет finalize)
                    goto process_char_again;
                }
            }
            break; // Конец case NUMBER_END_POTENTIAL_SUFFIX
                 break; // Конец case NUMBER_END_POTENTIAL_SUFFIX

            // ---> Обработка состояний суффикса <---
             case SUFFIX_U: // Прочитали ...u
                 if (tolower(c) == 'l') { num_state = SUFFIX_UL; current_token += c;}
                 else { current_token += c; num_state = INVALID; } // Любой другой символ - ошибка
                 break;                                             // (Разделители обработаны выше)
             case SUFFIX_L: // Прочитали ...l
                 if (tolower(c) == 'l') { num_state = SUFFIX_LL; current_token += c;}
                 else if (tolower(c) == 'u') { num_state = SUFFIX_UL; current_token += c;}
                 else { current_token += c; num_state = INVALID; }
                 break;
            case SUFFIX_LL: // Прочитали ...ll
                 if (tolower(c) == 'u') { num_state = SUFFIX_ULL; current_token += c;}
                 else { current_token += c; num_state = INVALID; }
                 break;
             case SUFFIX_UL: // Прочитали ...ul или ...lu
                 if (tolower(c) == 'l') { num_state = SUFFIX_ULL; current_token += c;} // ul + l = ull
                 else { current_token += c; num_state = INVALID; }
                 break;
             case SUFFIX_ULL: // Прочитали ...ull или ...llu
                 // Любой символ после этого - ошибка
                 current_token += c; num_state = INVALID;
                 break;

        } // Конец switch(num_state)

    } // Конец while(in.get(c))

    // Финализация последнего токена после выхода из цикла
    finalize_token();

    in.close();
    report_out.close();

    std::cout << "Report generated successfully." << std::endl; // Сообщение пользователю

    return 0;
}