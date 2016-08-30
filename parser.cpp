/*

Copyright (c) 2016 Louai Al-Khanji

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "noldor_impl.h"

#include <sstream>

namespace noldor {

static bool is_symbol_initial(int c)
{
    return isalpha(c) || strchr("!$%&*/:<=>?^_~", c);
}

static bool is_symbol_subsequent(int c)
{
    return is_symbol_initial(c) || isdigit(c) || strchr("+-.@", c);
}

static bool is_delimiter(int c)
{
    return isspace(c) || strchr("()\";", c);
}

static std::string parser_escape_string(std::string s)
{
    for (auto i = s.find('\\'); i != decltype(s)::npos; i = s.find('\\', i + 1)) {
        auto next = i + 1;
        if (next == s.size()) {
            // hmm the string ends in a backslash, what do? let's just remove it for now...
            s.erase(i, 1);
            continue;
        }

        switch (s[next]) {
        case ' ':
        case '\t': {
            bool saw_new_line = false;

            for (++next; next < s.size() && strchr(" \t\n", s[next]) && (s[next] != '\n' || !saw_new_line); ++next)
                saw_new_line = saw_new_line || s[next] == '\n';

            s.erase(i, next - i);
            continue;
        }

        case '"':
        case '\\':
        case '|':
            s.erase(i, 1);
            continue;

        case 't':
            s.erase(i, 1);
            s[i] = '\t';
            continue;

        case 'n':
            s.erase(i, 1);
            s[i] = '\n';
            continue;

        case 'r':
            s.erase(i, 1);
            s[i] = '\r';
            continue;

        case 'x': {
                const auto semicolon = s.find(';', next);
                if (semicolon == decltype(s)::npos)
                    continue;

                const auto sub = s.substr(next+1, semicolon - next-1);
                for (auto c : sub) {
                    if (!isxdigit(c)) {
                        std::cerr << "warning: bad hex escape \\x" << sub << "; ignored" << std::endl;
                        continue;
                    }
                }

                const auto hexvalue = std::stoi(sub, nullptr, 16);
                s.replace(i, 3 + sub.size(), 1, static_cast<decltype(s)::value_type>(hexvalue));
                continue;
            }
        }
    }

    return s;
}

static std::vector<value> read_list_or_vector(std::istream &input)
{
    char c;

    auto skipspace = [&] {
        while (isspace(input.peek()) && input.get(c))
            ;
        return input.peek();
    };

    std::vector<value> elements;

    while (input) {
        if (skipspace() == ')') {
            input.get(c);
            return elements;
        }

        elements.push_back(read(input));
    }

    return elements;
}

value read(std::istream &input)
{
    char c;

    while (input.get(c)) {
        if (isspace(c))
            continue;

        if (c == '.') {
            char dotdot[2] = { 0, 0 };

            if (input.read(dotdot, 2) && strncmp("..", dotdot, 2) == 0) {
                return SYMBOL_LITERAL(...);
            } else {
                switch (input.gcount()) {
                case 2:
                    input.putback(dotdot[1]);
                case 1:
                    input.putback(dotdot[0]);
                }

                return SYMBOL_LITERAL(.);
            }
        }

        if (is_symbol_initial(c)) {
            std::string symname(1, c);
            while (is_symbol_subsequent(input.peek()) && input.get(c))
                symname += c;
            return symbol(symname);
        }

        if (c == '+' && is_delimiter(input.peek()))
            return SYMBOL_LITERAL(+);

        if (c == '-' && is_delimiter(input.peek()))
            return SYMBOL_LITERAL(-);

        if (isdigit(c) || ((c == '+' || c == '-') && isdigit(input.peek()))) {
            const int sign = (c == '-') ? -1 : 1;

            if (isdigit(c))
                input.putback(c);

            int32_t num;
            input >> num;

            if (input.peek() != '.')
                return mk_int(num * sign);

            input.get(c);
            double fraction = 0;

            if (isdigit(input.peek()))
                input >> fraction;

            while (fraction > 1)
                fraction = fraction / 10.0;

            const double n = (double(num) + fraction) * sign;
            return mk_double(n);
        }

        switch (c) {
        case ';':
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;

        case '\'':
            return list(SYMBOL_LITERAL(quote), read(input));

        case '`':
            return list(SYMBOL_LITERAL(quasiquote), read(input));

        case ',':
            if (input.peek() == '@') {
                input.get(c);
                return list(SYMBOL_LITERAL(unquote-splicing), read(input));
            } else {
                return list(SYMBOL_LITERAL(unquote), read(input));
            }
            break;

        case '"':
        {
            std::string string, sbuf;
            while (std::getline(input, sbuf, '"')) {
                string += sbuf;
                if (string.back() == '\\')
                    string.pop_back();
                else
                    break;
            }
            return mk_string(parser_escape_string(string));
        }
            break;

        case '(': {
            const std::vector<value> elems = read_list_or_vector(input);
            auto list = noldor::list();

            for (auto i = elems.rbegin(); i != elems.rend(); ++i)
                list = cons(*i, list);

            return list;
        }

        case ')':
            throw parse_error("parser error: unexpected closing paren");
            break;

        case '#': {
            if (input.get(c)) switch (c) {
            case 't':
            case 'f':
            case '\\': {
                std::string word = "#" + std::string(1, c);
                while (!is_delimiter(input.peek()) && input.get(c))
                    word += std::string(1, c);

                if (word == "#t" || word == "#true") {
                    return mk_bool(true);
                } else if (word == "#f" || word == "#f") {
                    return mk_bool(false);
                } else if (word[1] == '\\') {
                    if (word.length() == 2 && input.get(c))
                        return mk_char(c);
                    else if (word.length() == 3)
                        return mk_char(word[2]);
                    else if (word == "#\\alarm")
                        return mk_char('\a');
                    else if (word == "#\\backspace")
                        return mk_char('\b');
                    else if (word == "#\\delete")
                        return mk_char('\x7f');
                    else if (word == "#\\escape")
                        return mk_char('\x1b');
                    else if (word == "#\\newline")
                        return mk_char('\n');
                    else if (word == "#\\null")
                        return mk_char('\0');
                    else if (word == "#\\return")
                        return mk_char('\r');
                    else if (word == "#\\space")
                        return mk_char(' ');
                    else if (word == "#\\tab")
                        return mk_char('\t');
                    else
                        throw parse_error("unknown named character " + word);
                } else {
                    throw parse_error("cannot parse # sequence " + word + " at position " + std::to_string(input.tellg()));
                }

                break;
            }

            case '(':
                return mk_vector(read_list_or_vector(input));

            }
        }
        }
    }

    return mk_eof_object();
}

} // namespace noldor
