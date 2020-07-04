///////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2020 nyfrk <nyfrk@gmx.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hlib.h>
using namespace hlib;

HANDLE WINAPI CreateFileW_Hook(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);

static JmpPatch CreateFileWPatch((UINT64)GetProcAddress(GetModuleHandleW(L"KERNELBASE.DLL"), "CreateFileW"), (DWORD)CreateFileW_Hook,(Patch::BYTE5 *)"\x8B\xFF\x55\x8B\xEC");
static const DWORD OrigRetAddr = (DWORD)CreateFileWPatch.getAddress() + 5;

HANDLE __declspec(naked) WINAPI CreateFileW_Orig(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile) {
	_asm {
		// prepare for future replacement with the orig 5 bytes
		mov edi, edi
		push ebp
		mov ebp, esp

		// back to orig
		jmp OrigRetAddr
	}
}

HANDLE WINAPI CreateFileW_Hook(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile) {

	// Be careful to not call any functions that may recursively call CreateFileW in this hook!

	static const struct { LPCWSTR from, to; } redirects[] = {
		{L"Gfx\\2.gh5", L"Gfx\\41.gh5"},
		{L"Gfx\\2.gh6", L"Gfx\\41.gh6"},
		{L"Gfx\\2.gl5", L"Gfx\\41.gl5"},
		{L"Gfx\\2.gl6", L"Gfx\\41.gl6"},
	};

	for (auto& r : redirects) {
		if (!wcscmp(lpFileName,r.from)) {
			lpFileName = r.to;
			break;
		}
	}

	return CreateFileW_Orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH: {
			if (!CreateFileWPatch.patch(GetCurrentProcess())) {
				MessageBoxA(NULL, "Cannot change textures because the target is already hooked. Please open an issue here to solve this incompatibility:\nhttps://github.com/nyfrk/Settlers4-TextureChanger/issues", "Texture Changer ASI Plugin", MB_ICONEXCLAMATION | MB_OK);
			}
			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH: {
			CreateFileWPatch.unpatch(GetCurrentProcess());
			break;
		}
    }
    return TRUE;
}

