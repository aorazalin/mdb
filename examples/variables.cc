struct MyStruct {
		char a;
		double b;
		float c;
		char *ptr;
};	

void myFactory() {
		MyStruct ms;
		ms.a = 'x';
		ms.b = 3.7;
		ms.c = -4.2;
		ms.ptr = &ms.a;
}

void manyVars() {
		int i = 4;
		//
		//
		bool b = true;
		double d = 4.124;
		float f = 12.21;
		myFactory();
//		char *c = "qwerty";
}

int main() {
		int x = 4;
		manyVars();
}
