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

#include "noldor.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

namespace noldor {

value load(std::string filename, dot_tag, value environment_specifier)
{
    if (!is_null(environment_specifier))
        environment_specifier = car(environment_specifier);

    check_type(is_environment, environment_specifier, "load: expected environment as second argument");

    value port = open_input_file(filename);
    basic_scope scope { &port };

    while (true) {
        value val = read(port);

        if (is_eof_object(val))
            break;

        eval(val, environment_specifier);
    }

    return SYMBOL_LITERAL(ok);
}


bool file_exists(std::string filename)
{
    struct stat buffer;
    return stat(filename.c_str(), &buffer) == 0;
}

bool delete_file(std::string filename)
{
    return remove(filename.c_str()) == 0;
}

static value command_line_args = list();
static basic_scope command_line_args_scope { &command_line_args };

void set_command_line(int argc, char **argv)
{
    command_line_args = list();

    for (int i = 0; i < argc; ++i)
        command_line_args = cons(mk_string(argv[i]), command_line_args);
}

value command_line()
{
    return command_line_args;
}

static int value_to_exit_code(value obj)
{
    if (is_null(obj))
        obj = mk_bool(true);
    else
        obj = car(obj);

    int result = EXIT_SUCCESS;

    if (is_int(obj))
        result = to_int(obj);
    else if (is_bool(obj))
        result = is_false(obj) ? EXIT_FAILURE : EXIT_SUCCESS;

    return result;
}

bool exit(dot_tag, value obj)
{
    ::exit(value_to_exit_code(obj));
}

bool emergency_exit(dot_tag, value obj)
{
    ::_Exit(value_to_exit_code(obj));
}

value get_environment_variable(std::string name)
{
    return mk_string(getenv(name.c_str()));
}

value get_environment_variables()
{
    value result = list();

    for (char **env = environ; env; ++env) {
        char *equals_sign = strchr(*env, '=');

        if (!equals_sign) {
            fprintf(stderr, "Invalid environ entry: %s\n", *env);
            continue;
        }

        auto name = std::string(*env, equals_sign - *env);
        auto val = std::string(equals_sign + 1);

        result = cons(cons(mk_string(std::move(name)), mk_string(std::move(val))), result);
    }

    return result;
}

double current_second()
{
    return double(current_jiffy()) / double(jiffies_per_second());
}

int32_t current_jiffy()
{
    struct timeval time;
    int32_t jiffy = -1;

    if (gettimeofday(&time, NULL) == 0)
        jiffy = time.tv_sec * jiffies_per_second() + time.tv_usec;

    return jiffy;
}

int32_t jiffies_per_second()
{
    return 1000;
}

}
