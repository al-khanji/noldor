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
