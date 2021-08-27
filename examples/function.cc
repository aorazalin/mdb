#include<iostream>

void foo() {
    std::cout << "FOO!\n";
    long a = 5;
    long b = a - 3;
    std::cout << "FOOEND\n";
}

int main() {
    foo();
}
