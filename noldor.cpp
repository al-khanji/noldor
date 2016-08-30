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
#include <iostream>

using namespace noldor;

int main(int, char **)
{
    noldor_init();

    auto env = mk_environment();

    scope sc;
    sc.add(env);

    while (std::cin) {
        try {
            std::cout << "\u03BB :: ";
            auto exp = read(std::cin);

            if (is_eof_object(exp)) {
                std::cout << std::endl << "\u203B" << std::endl;
                break;
            }

            auto result = eval(exp, env);
            std::cout << "  \u2971 " << result << std::endl;
        } catch (std::exception &e) {
            std::cerr << std::endl << e.what() << std::endl;
        }
    }

    return 0;
}