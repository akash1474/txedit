#include "iostream"
#include "Python.h"

int main(){
	Py_Initialize();
	PyRun_SimpleString("a=10\nb=30\nprint('Hello from PY...')\nprint(a+b)");
	Py_Finalize();
	return 0;
}