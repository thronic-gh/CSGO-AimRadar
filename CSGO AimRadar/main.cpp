#include "main.h"

/*
 *	Gjenoppliver min første aimbot opprinnelig laget for CS:S. 
 *	Tilpasser den til CS:GO ved hjelp av hazedump sine data.
 *	Tar også tid og mulighet til å rydde litt i koden.
 * 
 *	Egenskaper (akkurat som med CS:S):
 *	- Autosikting med museknapp 5 (VK_XBUTTON1, Navigasjonsknapp).
 *	- Tilpasset 9° akseptabel siktevinkel, for å kontrollere snapping.	
 *	- No Recoil.
 *	- 3D Radar (ESP).
 * 
 * (c) 2022 ժʝ_
 * 
 * Ressurser. Jeg låner offsets og mønstre fra lenkene nedenfor.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * https://github.com/frk1/hazedumper/blob/master/config.json
 * https://github.com/frk1/hazedumper/blob/master/csgo.cs
 */




LRESULT __stdcall WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) 
{
	switch (Msg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}

	return 0;
}




int __stdcall wWinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, 
	PWSTR lpCmdLine, 
	int nCmdShow
) {
	WNDCLASSEX wc;
	HWND hWnd;
	MSG Msg;

	wc.cbSize = sizeof(WNDCLASSEX);	
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = 0;	
	wc.lpszClassName = L"MainClass";

	if (!RegisterClassEx(&wc)) {
		GetError(L"wWinMain 1 Error. Exiting.");
		exit(EXIT_FAILURE);
	}

	hWnd = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TRANSPARENT,
		L"MainClass",
		L"Thronic CSGO AimRadar",
		WS_POPUPWINDOW,
		CW_USEDEFAULT, /* int X position. */
		CW_USEDEFAULT, /* int Y position. */
		240, /* int Width. */
		120, /* int Height. */
		0,
		0,
		hInstance,
		0
	);

	if (!hWnd) {
		GetError(L"wWinMain 2 Error. Exiting.");
		exit(EXIT_FAILURE);
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	ProjectorHwnd = &hWnd;
	hdc = GetDC(hWnd);

	//
	// Forsyner prosjektoren vår med gjennomsiktigheten den trenger.
	// Vindustiler satt via CreateWindowEx() tar seg av det funksjonelle.
	// WS_POPUPWINDOW forsyner en viss ramme.
	// WS_EX_LAYERED er krevd av funksjonen under.
	// WS_EX_TRANSPARENT gjør at vi kan samhandle med spillet.
	//
	SetLayeredWindowAttributes(hWnd, RGB(255, 255, 255), NULL, LWA_COLORKEY);


	//
	// Sett tilgang til å lese minnet til andre programmer. Finn spillprosess 
	// og sett opp minnepekere til DLL filer vi trenger å hente minnedata fra. 
	//
	try {
		hProc.PrepProcess(L"csgo.exe");
	}
	catch (...) {
		GetError(L"Make sure the game is running before starting this.");
		exit(EXIT_FAILURE);
	}


	// Hovedloop.
	while(1) {
		Sleep(1); // CPU. 

		// Håndter systemmeldinger til vinduet, diskré.
		while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		} 

		// Avslutt hvis WM_QUIT har blitt postet.
		if (Msg.message == WM_QUIT)
			break;

		// Oppdater størrelse og posisjon.
		try { UpdateLayerStatus(); } catch(...) { 
			GetError(L"UpdateLayerStatus() Error. Exiting."); 
			exit(EXIT_FAILURE);
		}

		// Oppdater minnedata.
		try { UpdateDataCollection(); } catch(...) { 
			GetError(L"UpdateDataCollection() Error. Exiting."); 
			exit(EXIT_FAILURE);
		}

		// Oppdater projisering.
		try { UpdateProjection(); } catch(...) { 
			GetError(L"UpdateProjection() Error. Exiting."); 
			exit(EXIT_FAILURE);
		}

		//
		// VK_XBUTTON1	0x05	X1 mouse button
		// Den bakerste/nærmeste navigasjonsknappen.
		//
		if (GetAsyncKeyState(0x05) & 0x8000)
			AimAtTarget();
	}

	return Msg.wParam;
}
