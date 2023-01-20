#pragma once

//
//	Generell feilhåndterer av exceptions o.l. i programmet.
//
void GetError(std::wstring lpszFunction)
{
	int err = GetLastError();
	std::wstring lpDisplayBuf;
	wchar_t* lpMsgBuf;

	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		0, 
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf, 
		0, 
		0
	);

	lpDisplayBuf.append(L"(" + std::to_wstring(err) + L") ");
	lpDisplayBuf.append(lpMsgBuf);

	std::wstring TotalMessage;
	TotalMessage.append(lpszFunction + L"\n\nSystem:\n" + lpDisplayBuf);

	MessageBoxW(
		0, 
		(LPCWSTR)TotalMessage.c_str(),
		L"Information",
		MB_OK | MB_ICONINFORMATION
	);
}

void GetHexError(uintptr_t s)
{
	std::wstringstream ss;

	ss << std::setfill(L'0') << std::setw(8) << std::hex << std::uppercase << s;

	MessageBoxW(
		0,
		(LPCWSTR)ss.str().c_str(),
		L"Information",
		MB_OK | MB_ICONINFORMATION
	);
}