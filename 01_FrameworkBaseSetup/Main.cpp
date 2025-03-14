#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "DummyClass.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	DummyClass dummy;
	dummy.DummyFunction();
	return 0;
}
