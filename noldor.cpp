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

using namespace noldor;

struct basic_scope : scope
{
    std::vector<value *> variables;

    void visit(gc_visit_fn_t visitor, void *data) override
    {
        for (value *v : variables)
            visitor(v, data);
    }
};

int main(int, char **)
{
    noldor_init();

    auto env = mk_environment();
    auto port = mk_input_port(0);

    basic_scope sc;
    sc.variables =  { &env, &port };

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
