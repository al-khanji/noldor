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
#include <poll.h>
#include <cassert>
#include <sys/socket.h>

namespace noldor {

enum : unsigned {
    port_file    = 0x01,
    port_string  = 0x02,
    port_input   = 0x04,
    port_output  = 0x08,
    port_textual = 0x10,
    port_binary  = 0x20,
    port_open    = 0x40,
    port_noclose = 0x80
};

struct port_t {
    port_t(const port_t &) = delete;
    port_t(port_t &&) = default;
    ~port_t();

    bool close_port();

    unsigned flags = 0;
    int fd = -1;
    std::string strdata; // filename if flags & port_file, data if flags & port_string
    std::vector<uint32_t> buffer; // pusback buffer
};

port_t::~port_t()
{}

bool port_t::close_port()
{
    if ((flags & port_open) == 0)
        return false;

    if ((flags & port_noclose)) {
        flags &= ~port_open;
        fd = -1;
        return true;
    }

    if (fd != -1) {
        if (close(fd) == -1)
            perror("close");
        fd = -1;
    }

    if (flags & port_string)
        strdata.clear();

    flags &= ~port_open;
    return true;
}

static void port_destruct(value port)
{
    object_data_as<port_t *>(port)->close_port();
    object_data_as<port_t *>(port)->~port_t();
}

static void port_gc_visit(value, gc_visit_fn_t, void*)
{}

static std::string port_repr(value port)
{
    auto data = object_data_as<port_t *>(port);
    std::stringstream repr;

    repr << "<#port";

    if (data->flags & port_file)
        repr << " " << data->strdata << " fd " << data->fd << " ";
    if (data->flags & port_string)
        repr << " stringbuf ";
    if (data->flags & port_open)
        repr << "open ";
    if (data->flags & port_noclose)
        repr << "noclose ";
    if (data->flags & port_input)
        repr << "r";
    if (data->flags & port_output)
        repr << "w";
    if (data->flags & port_binary)
        repr << "b";

    repr << ">";

    return repr.str();
}

static metatype_t *port_metaobject()
{
    static metatype_t metaobject = {
        METATYPE_VERSION,
        typeflags_none,
        port_destruct,
        port_gc_visit,
        port_repr
    };

    return &metaobject;
}

bool is_input_port(value val)
{
    return is_port(val) && object_data_as<port_t *>(val)->flags & port_input;
}

bool is_output_port(value val)
{
    return is_port(val) && object_data_as<port_t *>(val)->flags & port_output;
}

bool is_textual_port(value val)
{
    return is_port(val) && (object_data_as<port_t *>(val)->flags & port_binary) == 0;
}

bool is_binary_port(value val)
{
    return is_port(val) && object_data_as<port_t *>(val)->flags & port_binary;
}

bool is_port(value val)
{
    return object_metaobject(val) == port_metaobject();
}

bool is_input_port_open(value val)
{
    check_type(is_input_port, val, "is_input_port_open: expected input port");
    return object_data_as<port_t *>(val)->flags & port_open;
}

bool is_output_port_open(value val)
{
    check_type(is_output_port, val, "is_output_port_open: expected output port");
    return object_data_as<port_t *>(val)->flags & port_open;
}

//value current_input_port()
//{}

//value current_output_port()
//{}

//value current_error_port()
//{}

bool is_file_port(value val)
{
    return is_port(val) && object_data_as<port_t *>(val)->flags & port_file;
}

static unsigned port_flags(int oflag)
{
    unsigned result = 0;

    if ((oflag & O_ACCMODE) == O_RDONLY)
        result |= port_input;

    if ((oflag & O_ACCMODE) == O_WRONLY)
        result |= port_output;

    if ((oflag & O_ACCMODE) == O_RDWR)
        result |= (port_input | port_output);

    return result;
}

static value open_file(std::string filename, int oflag)
{
    int fd = open(filename.c_str(), oflag | O_CLOEXEC, 0644);
    if (fd == -1) {
        perror("open");
        throw file_error(strerror(errno), mk_string(std::move(filename)));
    }

    return object_allocate<port_t>(port_metaobject(),
                                   port_t { port_file | port_open | port_flags(oflag),
                                            fd, std::move(filename), {} });
}

value mk_input_port(int fd)
{
    return object_allocate<port_t>(port_metaobject(),
                                   port_t { port_file | port_open | port_input | port_binary | port_textual | port_noclose,
                                            fd, "", {} });
}

value mk_input_port(FILE *fp)
{
    assert(fp);
    return object_allocate<port_t>(port_metaobject(),
                                   port_t { port_file | port_open | port_input | port_binary | port_textual | port_noclose,
                                            fileno(fp), "", {} });
}

value open_input_file(std::string filename)
{
    value val = open_file(std::move(filename), O_RDONLY);
    object_data_as<port_t *>(val)->flags |= port_binary | port_textual;
    return val;
}

value open_binary_input_file(std::string filename)
{
    value val = open_file(std::move(filename), O_RDONLY);
    object_data_as<port_t *>(val)->flags |= port_binary | port_textual;
    return val;
}

value open_output_file(std::string filename)
{
    value val = open_file(std::move(filename), O_CREAT | O_WRONLY);
    object_data_as<port_t *>(val)->flags |= port_binary | port_textual;
    return val;
}

value open_binary_output_file(std::string filename)
{
    value val = open_file(std::move(filename), O_CREAT | O_WRONLY);
    object_data_as<port_t *>(val)->flags |= port_binary | port_textual;
    return val;
}

bool close_port(value val)
{
    check_type(is_port, val, "close_port: expected port");
    return object_data_as<port_t *>(val)->close_port();
}

bool close_input_port(value val)
{
    check_type(is_input_port, val, "close_input_port: expected input port");
    return object_data_as<port_t *>(val)->close_port();
}

bool close_output_port(value val)
{
    check_type(is_output_port, val, "close_output_port: expected output port");
    return object_data_as<port_t *>(val)->close_port();
}

bool is_string_port(value val)
{
    return is_port(val) && object_data_as<port_t *>(val)->flags & port_string;
}

value open_input_string(std::string string)
{
    return object_allocate<port_t>(port_metaobject(),
                                   port_t { port_string | port_textual | port_input | port_open,
                                            -1, std::move(string), {} });
}

value open_output_string()
{
    return object_allocate<port_t>(port_metaobject(),
                                   port_t { port_string | port_textual | port_output | port_open,
                                            -1, "", {} });
}

std::string get_output_string(value val)
{
    check_type(is_output_port, val, "get_output_string: expected output port");

    auto port = object_data_as<port_t *>(val);
    if ((port->flags & port_string) == 0)
        throw type_error("expected string port", val);

    return port->strdata;
}

static value get_input_port(std::string fname, value portl)
{
    value val = list();

    if (is_null(portl) == false) {
        if (is_null(cdr(portl)) == false)
            throw noldor::call_error("expected single argument", portl);

        val = car(portl);
    }

    fname += ": expected input port";

    check_type(is_input_port, val, fname.c_str());

    return val;
}

static bool is_symbol_initial(int c)
{
    return isalpha(c) || strchr("!$%&*/:<=>?^_~@", c);
}

static bool is_symbol_subsequent(int c)
{
    return is_symbol_initial(c) || isdigit(c) || strchr("+-.@", c);
}

static bool is_symbol_subsequent(value c)
{
    return is_char(c) && is_symbol_subsequent(char_get(c));
}

static bool is_delimiter(int c)
{
    return isspace(c) || strchr("()\";", c);
}

static bool is_delimiter(value c)
{
    return is_char(c) && is_delimiter(char_get(c));
}

static bool is_peek_char(value port, char ch)
{
    value peeked = peek_char(port);
    return is_char(peeked) && char_get(peeked) == uint32_t(ch);
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

static std::vector<value> read_list_or_vector(value port)
{
    std::vector<value> elements;

    while (true) {
        value chval = peek_char(port);

        if (is_eof_object(chval))
            throw parse_error("unexpected eof while reading list or vector");

        if (isspace(char_get(chval))) {
            read_char(port);
            continue;
        }

        if (char_get(chval) == ')') {
            read_char(port);
            return elements;
        }

        elements.push_back(read(port));
    }
}

value read(dot_tag, value portl)
{
    value port = get_input_port("read", portl);
    return read(port);
}

value read(value port)
{
    if (!is_input_port_open(port))
        return mk_eof_object();

    std::string token;

    while (true) {
        value val = read_char(port);

        if (is_eof_object(val))
            return val;

        auto ch = char_get(val);

        if (isspace(ch))
            continue;

        if (ch == '.') {
            if (!is_peek_char(port, '.'))
                return SYMBOL_LITERAL(.);

            read_char(port);
            if (!is_peek_char(port, '.'))
                return SYMBOL_LITERAL(..);

            read_char(port);
            return SYMBOL_LITERAL(...);
        }

        if (is_symbol_initial(ch)) {
            std::string symname(1, char(ch));
            while (is_symbol_subsequent(peek_char(port)))
                symname += char(char_get(read_char(port)));
            return symbol(symname);
        }

        if (ch == '+' && is_delimiter(peek_char(port)))
            return SYMBOL_LITERAL(+);

        if (ch == '-' && is_delimiter(peek_char(port)))
            return SYMBOL_LITERAL(-);

        if (isdigit(ch) || ((ch == '+' || ch == '-') && isdigit(char_get(peek_char(port))))) {
            std::string numstr;
            numstr.insert(numstr.begin(), ch);

            while (true) {
                value next = peek_char(port);
                if (is_eof_object(next))
                    break;
                char cc = char_get(next);
                if (!isdigit(cc))
                    break;
                numstr += cc;
                read_char(port);
            }

            if (!is_peek_char(port, '.'))
                return mk_int(strtol(numstr.c_str(), NULL, 10));

            numstr += '.';
            read_char(port);

            while (true) {
                value next = peek_char(port);
                if (is_eof_object(next))
                    break;
                char cc = char_get(next);
                if (!isdigit(cc))
                    break;
                numstr += cc;
                read_char(port);
            }

            return mk_double(strtod(numstr.c_str(), NULL));
        }

        switch (ch) {
        case ';':
            read_line(port);
            continue;

        case '\'':
            return list(SYMBOL_LITERAL(quote), read(port));

        case '`':
            return list(SYMBOL_LITERAL(quasiquote), read(port));

        case ',':
            if (is_peek_char(port, '@')) {
                read_char(port);
                return list(SYMBOL_LITERAL(unquote-splicing), read(port));
            } else {
                return list(SYMBOL_LITERAL(unquote), read(port));
            }

        case '"': {
            std::string string;
            while (true) {
                value ch = read_char(port);
                if (is_eof_object(ch))
                    throw parse_error("unexpected eof while parsing string: " + string);

                if (char_get(ch) == uint32_t('"') && string.back() != '\\')
                    return mk_string(parser_escape_string(string));

                string.append(1, char_get(ch));
            }
        }

        case '(': {
            const std::vector<value> elems = read_list_or_vector(port);
            auto list = noldor::list();

            for (auto i = elems.rbegin(); i != elems.rend(); ++i)
                list = cons(*i, list);

            return list;
        }

        case ')':
            throw parse_error("parser error: unexpected closing paren");

        case '#': {
            value next = read_char(port);
            if (is_eof_object(next))
                throw parse_error("parser error: unexpected eof after #");

            char c = char(char_get(next));
            switch (c) {
            case 't':
            case 'f':
            case '\\': {
                std::string word = "#" + std::string(1, c);

                while (true) {
                    next = peek_char(port);
                    if (is_eof_object(next) || is_delimiter(next))
                        break;

                    read_char(port);
                    word += std::string(1, char_get(next));
                }

                if (word == "#t" || word == "#true")
                    return mk_bool(true);
                else if (word == "#f" || word == "#f")
                    return mk_bool(false);
                else if (word[1] == '\\') {
                    if (word.length() == 2) {
                        next = read_char(port);
                        if (!is_eof_object(next))
                            return mk_char(char_get(next));
                    }

                    if (word.length() == 3)
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
                    throw parse_error("cannot parse # sequence " + word);
                }
            }

            case '(':
                return mk_vector(read_list_or_vector(port));

            }
        }
        }
    }
}

value read_char(value port)
{
    if (!is_input_port_open(port))
        return mk_eof_object();

    auto data = object_data_as<port_t *>(port);

    if (data->buffer.size() > 0) {
        uint32_t c = data->buffer.front();
        data->buffer.erase(data->buffer.begin());
        return mk_char(c);
    }

    if (is_string_port(port)) {
        if (data->strdata.empty())
            return mk_eof_object();

        uint32_t c = data->strdata.front();
        data->strdata.erase(0, 1);

        return mk_char(c);
    }

    if (is_file_port(port)) {
        char c;

        switch (::read(data->fd, &c, 1)) {
        case 0:
            return mk_eof_object();
        case 1:
            return mk_char(uint32_t(c));
        case -1:
            perror("read");
            throw file_error(std::string("read_char: ") + strerror(errno), port);
        }

        throw runtime_error("INTERNAL ERROR IN read_char.");
    }

    throw file_error("unknown port type", port);
}

value read_char(dot_tag, value portl)
{
    return read_char(get_input_port("read_char", portl));
}

value peek_char(value port)
{
    if (!is_input_port_open(port))
        return mk_eof_object();

    auto val = read_char(port);
    if (is_eof_object(val))
        return val;

    auto data = object_data_as<port_t *>(port);
    data->buffer.push_back(char_get(val));

    return val;
}

value peek_char(dot_tag, value portl)
{
    return peek_char(get_input_port("peek_char", portl));
}

value read_line(value port)
{
    if (!is_input_port_open(port))
        return mk_eof_object();

    auto data = object_data_as<port_t *>(port);

    if (is_string_port(port)) {
        if (data->strdata.empty())
            return mk_eof_object();

        auto it = std::find(data->strdata.begin(), data->strdata.end(), '\n');
        std::string str(data->strdata.begin(), it);
        data->strdata = std::string(it, data->strdata.end());

        return mk_string(std::move(str));
    }

    if (is_file_port(port)) {
        std::string line;
        char c;
        ssize_t result;

        while ((result = ::read(data->fd, &c, 1)) == 1 && c != '\n')
            line += c;

        if (line.size() > 0)
            return mk_string(line);

        if (result == 0)
            return mk_eof_object();

        throw file_error(strerror(errno), port);
    }

    throw file_error("unknown port type", port);
}

value read_line(dot_tag, value portl)
{
    return read_line(get_input_port("read_line", portl));
}

bool is_char_ready(value port)
{
    if (!is_input_port_open(port))
        return false;

    auto data = object_data_as<port_t *>(port);

    if (is_string_port(port))
        return data->strdata.size() > 0;

    if (is_file_port(port)) {
        int result;
        struct pollfd pfd = { data->fd, POLLIN | POLLPRI, 0 };
        EINTR_SAFE(result, ::poll, &pfd, 1, 0);
        if (result == -1) {
            perror("poll");
            throw file_error(strerror(errno), port);
        }

        return pfd.revents & (POLLIN | POLLPRI);
    }

    throw file_error("unknown port type", port);
}

bool is_char_ready(dot_tag, value portl)
{
    return is_char_ready(get_input_port("is_char_ready", portl));
}

value write(value obj, dot_tag, value port)
{
    port = is_null(port) ? current_output_port()
                         : cadr(port);

    check_type(is_output_port, port, "write: expected output port");

    auto repr = printable(obj);
    auto data = object_data_as<port_t *>(port);

    if (is_string_port(port)) {
        data->strdata += repr;
    } else if (is_file_port(port)) {
        while (repr.size()) {
            int result;
            EINTR_SAFE(result, ::write, data->fd, repr.data(), repr.size());

            if (result == -1) {
                perror("write");
                throw file_error(strerror(errno), port);
            }
        }
    }

    return obj;
}

value display(value obj, dot_tag, value port)
{
    port = is_null(port) ? current_output_port()
                         : cadr(port);

    check_type(is_output_port, port, "display: expected output port");

    auto repr = printable(obj);
    auto data = object_data_as<port_t *>(port);

    if (is_string_port(port)) {
        data->strdata += repr;
    } else if (is_file_port(port)) {
        while (repr.size()) {
            int result;
            EINTR_SAFE(result, ::write, data->fd, repr.data(), repr.size());

            if (result == -1) {
                perror("write");
                throw file_error(strerror(errno), port);
            }
        }
    }

    return obj;
}

value newline(dot_tag, value port)
{
    return display(mk_string("\n"), {}, port);
}

} // namespace noldor
