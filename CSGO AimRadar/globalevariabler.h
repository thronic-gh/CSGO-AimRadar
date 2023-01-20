#pragma once

#define PI 3.14159265359
#define MAX_PLAYER_COUNT 64

/*
	sv_cheats 1
	bot_stop 1

	m_lifeState					0 = alive, >0 død eller ikke relevant.
	m_iHealth					2-100 (1 = død).
	m_iTeamNum					1=Spec, 2=Terr, 3=CT
	m_bDormant					false = i nærheten. true = borte. Til ESP rasjonalisering.
	m_Local + m_aimPunchAngle	recoil x,y floats som kan trekkes fra dwClientState_ViewAngles.
*/

// "client.dll"+
uintptr_t dwLocalPlayer = 0xDE7964;				// 8D 34 85 ? ? ? ? 89 15 ? ? ? ? 8B 41 08 8B 48 04 83 F9 FF	+4 i sluttverdi
uintptr_t dwEntityList = 0x4DFCE74;				// BB ? ? ? ? 83 FF 01 0F 8C ? ? ? ? 3B F8
uintptr_t dwViewMatrix = 0x4DEDCA4;				// 0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9							+176

// Offsets til spillere.
uintptr_t m_lifeState = 0x25F;
uintptr_t m_iHealth = 0x100;
uintptr_t m_iTeamNum = 0xF4;
uintptr_t m_vecViewOffset = 0x108;
uintptr_t m_dwBoneMatrix = 0x26A8;
uintptr_t m_vecOrigin = 0x138;
uintptr_t m_aimPunchAngle = 0x303C;
uintptr_t m_bDormant = 0xED;					// 8A 81 ? ? ? ? C3 32 C0										+8

// "engine.dll"+
uintptr_t dwClientState = 0x59F194;				// A1 ? ? ? ? 33 D2 6A 00 6A 00 33 C9 89 B0
uintptr_t dwClientState_ViewAngles = 0x4D90;	// F3 0F 11 86 ? ? ? ? F3 0F 10 44 24 ? F3 0F 11 86
uintptr_t dwClientState_PlayerInfo = 0x52C0;	// 8B 89 ? ? ? ? 85 C9 0F 84 ? ? ? ? 8B 01

// Diverse
COLORREF Red = RGB(255, 0, 0);
COLORREF Black = RGB(0, 0, 0);
COLORREF Blue = RGB(0, 0, 255);
COLORREF CTBlue = RGB(102, 178, 255);
COLORREF White = RGB(224, 224, 224);
COLORREF TrueWhite = RGB(255,255,255);
COLORREF CYAN = RGB(20, 215, 215);
HDC hdc, BufferDC;
HWND* ProjectorHwnd;
RECT GameWindowRect;
RECT GameClientRect;
int AntallSpillereFunnet = 0;