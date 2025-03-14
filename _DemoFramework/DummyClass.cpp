#include "DummyClass.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void DummyClass::DummyFunction()
{
	OutputDebugString(L"DummyFunction called\n");
}
