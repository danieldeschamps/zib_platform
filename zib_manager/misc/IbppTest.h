class Test
{
	// Class 'Test' drives all the tests of this module.

	bool _Success;				// if true after all tests, everything succeeded
	int _WriteMode;				// 0 == default, 1 == speed, 2 == safety

	void Test1();
	void Test2(); 
	void Test3();
	void Test4();
	void Test5();
	void Test6();
	void Test7();
	void Test8();

public:
	void RunTests();
	int PrintResult();
	Test(int argc, char* argv[]);
	~Test();
};