void foo4() {
		int a = 3;
}

void foo3() {
		int a = 4;
		foo4();
}

void foo2() {
		int a = 5;
		foo3();
}

void foo1() {
		int a = 6;
		foo2();
}

int main() {
		foo1();
}
