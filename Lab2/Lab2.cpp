#include <iostream>
#include <fstream>
#include <string>
#include <cctype>

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
    IDLE, // Начальное состояние (не число)
    START_ZERO, // Встретили '0'
    OCTAL, // Внутри восьмеричного числа (после '0')
    DECIMAL, // Внутри десятичного числа
    HEX_START, // Встретили '0x' или '0X'
    HEX, // Внутри шестнадцатеричного числа
    NUMBER_END_POTENTIAL_SUFFIX, // Числовая часть закончилась, след. символ - буква
    SUFFIX_U, // Встретили 'u' в суффиксе
    SUFFIX_L, // Встретили 'l' в суффиксе
    SUFFIX_LL, // Встретили 'll' в суффиксе
    SUFFIX_UL, // Встретили 'ul' или 'lu'
    SUFFIX_ULL, // Встретили 'ull' или 'llu'
    INVALID // Недопустимая последовательность
};

// Функция для определения типа константы по суффиксам
// (l_count: 0 для нет 'l', 1 для 'l', 2 для 'll')
std::string get_int_type(bool has_u, int l_count)
{
    if (has_u)
    {
        if (l_count == 0) return "unsigned int";
        if (l_count == 1) return "unsigned long";
        return "unsigned long long"; // l_count = 2
    }
    if (l_count == 0) return "int"; // Тип по умолчанию
    if (l_count == 1) return "long";
    return "long long"; // l_count = 2
}

// Функция для проверки, является ли символ разделителем (завершает токен)
bool is_delimiter(char c)
{
    return std::isspace(c) || std::string("+-*/%=(){}[];,<>&|^!~?#:").find(c) != std::string::npos;
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
    bool has_u = false;
    int l_count = 0;
    bool saw_digit = false;

    // --- Лямбда-функция для финализации токена ---
    // Вызывается ПЕРЕД обработкой символа, который ЗАВЕРШАЕТ токен
    auto finalize_token = [&]()
    {
        if (current_token.empty())
        {
            // Не выводим ничего для пустого токена
            num_state = IDLE;
            potential_suffix_char = 0;
            has_u = false;
            l_count = 0;
            saw_digit = false;
            return;
        }
        // --- Основная логика анализа ---
        // 1. Если автомат уже в состоянии INVALID, это точно ошибка
        if (num_state == INVALID)
        {
            report_out << current_token << "\tERROR" << std::endl;
        }
        else
        {
            // Считаем, что если не INVALID, то автомат уже определил тип и валидность
            // Здесь просто выводим результат
            report_out << current_token << "\t" << get_int_type(has_u, l_count) << std::endl;
        }
        // --- Сброс состояния и токена в конце финализации ---
        current_token = "";
        num_state = IDLE;
        potential_suffix_char = 0;
        has_u = false;
        l_count = 0;
        saw_digit = false;
    };
    // --- Конец лямбда-функции finalize_token ---

    // --- Основной цикл чтения файла ---
    while (in.get(c))
    {
        // Вспомогательная переменная для повторной обработки символа
        char char_to_reprocess = 0;

    process_char_again: // Метка для повторной обработки

        // Если есть символ для повторной обработки, используем его
        if (char_to_reprocess != 0)
        {
            c = char_to_reprocess;
        }

        // 0. Обработка смены основного состояния (комментарии, строки)
        if (state == NORMAL)
        {
            // Проверяем, не начинается ли комментарий или строка
            if (c == '/')
            {
                // Может быть комментарий ИЛИ завершение числа перед оператором
                finalize_token(); // Завершаем предыдущий токен (если был)
                state = SLASH; // Переходим в состояние проверки '/'
                continue; // Переходим к след. итерации для обработки '/' в SLASH
            }
            if (c == '"')
            {
                finalize_token();
                state = IN_STRING;
                continue;
            }
            if (c == '\'')
            {
                finalize_token();
                state = IN_CHAR;
                continue;
            }
            // Если это не начало комм/строки, остаемся в NORMAL
        }
        else
        {
            // Мы НЕ в NORMAL, обрабатываем комментарии/строки и т.д.
            // (Логика как в Лаб 1, но без вывода символов)
            switch (state)
            {
            case SLASH:
                if (c == '*') state = MULTI_COMMENT;
                else if (c == '/') state = SINGLE_COMMENT;
                else
                {
                    // Был оператор деления
                    state = NORMAL;
                    // Оператор '/' был проигнорирован, теперь обрабатываем 'c' в NORMAL
                    char_to_reprocess = c; // Повторно обработаем 'c'
                    goto process_char_again;
                }
                break;
            // ... остальные case для комментариев/строк ...
            case MULTI_COMMENT:
                if (c == '*') state = STAR_IN_MULTI_COMMENT;
                break;
            case STAR_IN_MULTI_COMMENT:
                if (c == '/') state = NORMAL;
                else if (c != '*') state = MULTI_COMMENT;
                break;
            case SINGLE_COMMENT:
                if (c == '\n' || c == '\r') state = NORMAL;
                break;
            case IN_STRING:
                if (c == '\\') state = SLASH_IN_STRING;
                else if (c == '"') state = NORMAL;
                break;
            case IN_CHAR:
                if (c == '\\') state = SLASH_IN_CHAR;
                else if (c == '\'') state = NORMAL;
                break;
            case SLASH_IN_STRING:
                state = IN_STRING;
                break;
            case SLASH_IN_CHAR:
                state = IN_CHAR;
                break;
            default:
                state = NORMAL;
                break; // На всякий случай
            }
            continue; // Пропускаем обработку числа для не-NORMAL состояний
        }

        // --- Обработка символа 'c' в состоянии NORMAL ---
        if (num_state == IDLE)
        {
            saw_digit = false;
            has_u = false;
            l_count = 0;
        }

        // 1. Обработка состояния INVALID
        if (num_state == INVALID)
        {
            if (is_delimiter(c))
            {
                // Разделитель завершает ошибочный токен
                finalize_token(); // Выведет ошибку и сбросит state в IDLE
                // Разделитель сам по себе игнорируется
            }
            else
            {
                // Считаем символ продолжением ошибочного токена
                current_token += c;
            }
            continue; // Переходим к следующей итерации
        }

        // 2. Проверка на завершение ВАЛИДНОГО токена разделителем
        // (Исключаем NUMBER_END_POTENTIAL_SUFFIX, т.к. там символ - буква)
        if (num_state != IDLE && num_state != NUMBER_END_POTENTIAL_SUFFIX && is_delimiter(c))
        {
            if (num_state == HEX_START && !saw_digit)
            {
                num_state = INVALID;
            }
            if (c == '.')
            {
                current_token += c; // Добавляем точку к токену
                num_state = INVALID; // Помечаем токен как ошибочный
            }
            finalize_token(); // Завершаем токен, выведет результат
            // Разделитель сам по себе игнорируется
            continue; // Переходим к следующей итерации
        }

        // 4. Основная логика переходов автомата чисел
        switch (num_state)
        {
        case IDLE:
            if (c == '0')
            {
                num_state = START_ZERO;
                current_token += c;
            }
            else if (std::isdigit(c))
            {
                num_state = DECIMAL;
                current_token += c;
                saw_digit = true;
            }
        // Иначе (буква, оператор и т.д.) - игнорируем, остаемся в IDLE
            break;

        case START_ZERO: // Мы прочитали '0'
            if (c == 'x' || c == 'X')
            {
                num_state = HEX_START;
                current_token += c;
                saw_digit = false;
            }
            else if (c >= '0' && c <= '7')
            {
                num_state = OCTAL;
                current_token += c;
                saw_digit = true;
            }
            else if (c == 'u' || c == 'U')
            {
                num_state = SUFFIX_U;
                current_token += c;
                has_u = true;
            }
            else if (c == 'l' || c == 'L')
            {
                num_state = SUFFIX_L;
                current_token += c;
                l_count = 1;
            }
            else if (is_delimiter(c))
            {
                // Завершение токена, finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case DECIMAL: // Внутри 1..9...
            if (std::isdigit(c))
            {
                current_token += c;
                saw_digit = true;
            }
            else if (c == 'u' || c == 'U')
            {
                num_state = SUFFIX_U;
                current_token += c;
                has_u = true;
            }
            else if (c == 'l' || c == 'L')
            {
                num_state = SUFFIX_L;
                current_token += c;
                l_count = 1;
            }
            else if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case OCTAL: // Внутри 0[0-7]...
            if (c >= '0' && c <= '7')
            {
                current_token += c;
                saw_digit = true;
            }
            else if (c == 'u' || c == 'U')
            {
                num_state = SUFFIX_U;
                current_token += c;
                has_u = true;
            }
            else if (c == 'l' || c == 'L')
            {
                num_state = SUFFIX_L;
                current_token += c;
                l_count = 1;
            }
            else if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case HEX_START: // Мы прочитали '0x'
            if (std::isxdigit(c))
            {
                num_state = HEX;
                current_token += c;
                saw_digit = true;
            }
            else if (is_delimiter(c))
            {
                if (!saw_digit)
                {
                    num_state = INVALID;
                }
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case HEX: // Внутри 0x[0-f]...
            if (std::isxdigit(c))
            {
                current_token += c;
                saw_digit = true;
            }
            else if (c == 'u' || c == 'U')
            {
                num_state = SUFFIX_U;
                current_token += c;
                has_u = true;
            }
            else if (c == 'l' || c == 'L')
            {
                num_state = SUFFIX_L;
                current_token += c;
                l_count = 1;
            }
            else if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        // ---> Обработка буквы после числа <---
        case NUMBER_END_POTENTIAL_SUFFIX:
            // ---> Обработка буквы после числа <---
            // 'potential_suffix_char' содержит букву, прочитанную на пред. шаге ('a')
            // 'c' - это символ, идущий ПОСЛЕ этой буквы (например, разделитель)
            {
                char first_letter = potential_suffix_char; // 'a'
                char first_letter_lower = tolower(first_letter);
                potential_suffix_char = 0; // Сбрасываем

                if (first_letter_lower == 'u')
                {
                    current_token += first_letter; // Добавляем 'u' к токену "12" -> "12u"
                    num_state = SUFFIX_U; // Переходим в состояние суффикса
                    char_to_reprocess = c; // Повторно обработаем 'c'
                    goto process_char_again;
                }
                if (first_letter_lower == 'l')
                {
                    current_token += first_letter; // Добавляем 'l' к токену "12" -> "12l"
                    num_state = SUFFIX_L;
                    char_to_reprocess = c;
                    goto process_char_again;
                }
                // Буква была не 'u' и не 'l'. Это ошибка для целочисленной константы.
                current_token += first_letter; // Добавляем 'a' к токену "12" -> "12a"
                num_state = INVALID; // Переходим в состояние ошибки
                char_to_reprocess = c; // Повторно обработаем 'c' в состоянии INVALID
                // (скорее всего, 'c' будет разделителем и вызовет finalize)
                goto process_char_again;
            }
        // Конец case NUMBER_END_POTENTIAL_SUFFIX

        // ---> Обработка состояний суффикса <---
        case SUFFIX_U: // Прочитали ...u
            if (c == 'l' || c == 'L')
            {
                if (l_count == 0)
                {
                    num_state = SUFFIX_UL;
                    current_token += c;
                    l_count = 1;
                }
                else
                {
                    num_state = INVALID;
                    current_token += c;
                }
            }
            else if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case SUFFIX_L: // Прочитали ...l
            if (c == 'l' || c == 'L')
            {
                if (l_count == 1)
                {
                    num_state = SUFFIX_LL;
                    current_token += c;
                    l_count = 2;
                }
                else
                {
                    num_state = INVALID;
                    current_token += c;
                }
            }
            else if (c == 'u' || c == 'U')
            {
                if (!has_u)
                {
                    num_state = SUFFIX_UL;
                    current_token += c;
                    has_u = true;
                }
                else
                {
                    num_state = INVALID;
                    current_token += c;
                }
            }
            else if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case SUFFIX_LL: // Прочитали ...ll
            if (c == 'u' || c == 'U')
            {
                if (!has_u)
                {
                    num_state = SUFFIX_ULL;
                    current_token += c;
                    has_u = true;
                }
                else
                {
                    num_state = INVALID;
                    current_token += c;
                }
            }
            else if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case SUFFIX_UL: // Прочитали ...ul или ...lu
            // После ul/lu разрешён только разделитель или один дополнительный l (чтобы получить строго ull/llu), но не ul*l* или lu*l*
            if (c == 'l' || c == 'L')
            {
                // Если уже был l_count == 1 (ul/lu), то ещё один l даёт ull/llu (l_count == 2), но запрещаем больше l
                if (l_count == 1 && !has_u)
                {
                    num_state = SUFFIX_ULL;
                    current_token += c;
                    l_count = 2;
                }
                else
                {
                    num_state = INVALID;
                    current_token += c;
                }
            }
            else if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
            break;

        case SUFFIX_ULL: // Прочитали ...ull или ...llu
            if (is_delimiter(c))
            {
                // finalize_token вызовется выше
            }
            else
            {
                num_state = INVALID;
                current_token += c;
            }
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
