#include "“ªº∂ø’º‰≈‰÷√∆˜.hpp"

int main()
{
	char* p = (char*) default_Alloc_template<0>::allocate(128);
	default_Alloc_template<0>::deallocate(p, 128);
	default_Alloc_template<0>::allocate(128);
	memset(p, 'a', sizeof(char) * 10);
	p[10] = '\0';
	return 0;
}