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
#include <stdio.h>
#include <iostream>
#include <fcntl.h>

using namespace noldor;

int repl()
{
    auto env = mk_environment();
    auto port = mk_port_from_fd(0, O_RDONLY);

    basic_scope sc { &env, &port };

    puts("\u262F");

    while (true) {
        try {
            printf("\u03BB :: ");
            fflush(stdout);
            auto exp = read(port);

            if (is_eof_object(exp))
                break;

            auto result = eval(exp, env);
            printf("  \u2971 %s\n", printable(result).c_str());
            fflush(stdout);
        } catch (std::exception &e) {
            fprintf(stderr, "\n%s\n", e.what());
            fflush(stderr);
        }
    }

    puts("\n\u203B");

    return 0;
}

int main(int argc, char **argv)
{
    noldor_init(argc, argv);

    if (argc == 1)
        return repl();

    for (int i = 0; i < argc; ++i)
        load(argv[i], {}, interaction_environment());

    return 0;
}
