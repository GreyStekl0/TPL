#include <iostream>
#include <fstream>

enum State
{
    NORMAL,
    SLASH,
    IN_SINGLE_COMMENT,
    IN_MULTI_COMMENT,
    STAR_IN_MULTI,
    IN_STRING,
    IN_CHAR
};

int main()
{
    const char* fileName = "lab01.example.utf8.c";
    const char* tempFileName = "temp_output.c";

    std::ifstream in(fileName);
    if (!in)
    {
        std::cerr << "Не удалось открыть input.txt" << std::endl;
        return 1;
    }

    std::ofstream out(tempFileName);
    if (!out)
    {
        std::cerr << "Не удалось открыть output.txt для записи" << std::endl;
        return 1;
    }

    State state = NORMAL;
    char c;

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
            else
            {
                out.put(c);
            }
            break;

        case SLASH:
            if (c == '/')
            {
                state = IN_SINGLE_COMMENT;
            }
            else if (c == '*')
            {
                state = IN_MULTI_COMMENT;
            }
            else
            {
                out.put('/');
                out.put(c);
                state = NORMAL;
            }
            break;

        case IN_SINGLE_COMMENT:
            if (c == '\n')
            {
                out.put(c);
                state = NORMAL;
            }
            break;

        case IN_MULTI_COMMENT:
            if (c == '*')
            {
                state = STAR_IN_MULTI;
            }
            break;

        case STAR_IN_MULTI:
            if (c == '/')
            {
                state = NORMAL;
            }
            else
            {
                state = IN_MULTI_COMMENT;
            }
            break;

        case IN_STRING:
            out.put(c);
            if (c == '\\')
            {
                if (in.get(c))
                {
                    out.put(c);
                }
            }
            else if (c == '"')
            {
                state = NORMAL;
            }
            break;

        case IN_CHAR:
            out.put(c);
            if (c == '\\')
            {
                if (in.get(c))
                {
                    out.put(c);
                }
            }
            else if (c == '\'')
            {
                state = NORMAL;
            }
            break;
        }
    }
    if (state == SLASH)
    {
        out.put('/');
    }

    in.close();
    out.close();

    std::rename(tempFileName, fileName);
    return 0;
}
