#include<iostream>


void log(std::string msg) {
    std::cout << msg << "\n";
}

long compute(long b, long c) {
    log("computing addition of b and c");
    return b + c;
}

int main() {
    long b = 3, c = -2;
    long a = compute(b, c); 
}
