#include <cstdio>
#include <cassert>
#include <vector>
#include <string>
#include <initializer_list>

#include "RefPtr.h"

using wfextra::RefPtr;

int main() {
    RefPtr<int> int_ptr1(10);
    RefPtr<int> int_ptr2 = int_ptr1;
    auto int_ptr3 = int_ptr2;
    int_ptr1.reset(0);

    printf("ptr1:%d ptr2:%d ptr3:%d\n", *int_ptr1, *int_ptr2, *int_ptr3);

    std::string s("str");
    RefPtr<std::string> str_ptr(s);
    RefPtr<std::vector<char>> vec_char_ptr;
    vec_char_ptr.reset(s.begin(), s.end());
    vec_char_ptr.reset(std::vector<char>());
    vec_char_ptr.reset({'x', 'y'});

    constexpr std::initializer_list<char> il{'a', 'b', 'c'};
    str_ptr.reset(il);

    printf("str_ptr:%s\n", str_ptr->c_str());
    printf("vec_char_ptr size:%zu\n", vec_char_ptr->size());

    RefPtr<double> d_ptr;
    printf("d_ptr empty:%c\n", d_ptr ? 'n' : 'y');

    return 0;
}
