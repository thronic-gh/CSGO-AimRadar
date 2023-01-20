#pragma once

#include <windows.h>
#include <string>
#include <strsafe.h>
#include <TlHelp32.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "globalevariabler.h"
#include "geterror.h"
#include "prep.h"
#include "resource.h"




struct AimRegistry
{
	float AimProximity = 5000.0f;
	float Distance = 5000.0f;
	int PlayerNum = 0;

	// Funksjon som lar oss sortere på distansen med std::sort.
	bool operator < (const AimRegistry &other) {
		if (Distance < other.Distance && AimProximity < other.AimProximity)
			return true;

		return AimProximity < other.AimProximity;
	}
};


struct WorldToScreenMatrix_t {
	float flMatrix[4][4];
};


struct AimBoneType {
	float X;
	char _pad1[0x9];
	float Y;
	char _pad2[0x9];
	float Z;
}; 




void readmem(uintptr_t adr, void *buf, size_t size)
{
	try {
		ReadProcessMemory(hProc.__HandleProcess, (const void*)adr, buf, size, 0);

	} catch (std::exception &err) {
		std::string errwhat = err.what();
		GetError(L"readmem(): "+ std::wstring(errwhat.begin(), errwhat.end()));
	}
}




struct Player
{
	WorldToScreenMatrix_t WorldToScreenMatrix;
	uintptr_t PlayerPtr;
	uintptr_t EnginePtr;
	int Team;
	int Health;
	bool Dormant;
	float MyPos[3];
	float MyEyesPos[3];
	float Angle[2];
	float PunchAngle[2];
	uintptr_t dwClientStatePtr;
	unsigned char LifeState;

	// Tar imot offsets.
	void UpdateInformation()
	{
		try {

		// Statisk peker til spillerbase og engine.
		readmem(hProc.ClientModulePtr + dwLocalPlayer, &PlayerPtr, sizeof(uintptr_t));
		readmem(hProc.EngineModulePtr, &EnginePtr, sizeof(uintptr_t));

		// Hvilket lag er vi på?
		readmem(PlayerPtr + m_iTeamNum, &Team, sizeof(int));

		// Hvor mye HP har vi?
		readmem(PlayerPtr + m_iHealth, &Health, sizeof(int));

		// Lever vi?
		readmem(PlayerPtr + m_lifeState, &LifeState, sizeof(unsigned char));

		// Er vi dormant?
		readmem(PlayerPtr + m_bDormant, &Dormant, sizeof(bool));

		// Hva er XYZ posisjonen vår?
		readmem(PlayerPtr + m_vecOrigin, MyPos, sizeof(float)*3);

		// Hva er XYZ posisjonen til øynene våre?
		readmem(PlayerPtr + m_vecViewOffset, MyEyesPos, sizeof(float)*3);

		// dwClientState
		readmem(hProc.EngineModulePtr + dwClientState, &dwClientStatePtr, sizeof(uintptr_t));

		// Hva er vinkelen vår?
		readmem(dwClientStatePtr + dwClientState_ViewAngles, Angle, sizeof(float)*2);

		// Hva er recoil vinkelen vår?
		readmem(PlayerPtr + m_aimPunchAngle, PunchAngle, sizeof(float)*2);

		// Prøv å hent view matrix. 
		readmem(hProc.ClientModulePtr + dwViewMatrix, &WorldToScreenMatrix, sizeof(WorldToScreenMatrix_t));

		} catch (...) { 
			GetError(
				L"Player.UpdateInformation() Error. "
				L"Exiting."
			); 
			exit(EXIT_FAILURE); 
		}
	}

	void AimAtEnemy(float* AimAngle)
	{
		try {
			WriteProcessMemory(
				hProc.__HandleProcess, 
				(void*)(dwClientStatePtr + dwClientState_ViewAngles), 
				AimAngle, 
				sizeof(float)*2, 
				NULL
			);
		}
		catch (...) { 
			GetError(
				L"Player.AimAtEnemy() Error. "
				L"Exiting."
			); 
			exit(EXIT_FAILURE); 
		}
	}

	//
	//	Siktefunksjon som tar 2 vektorer og kalkulerer en skytevinkel. 
	//	Vi bruker pytagoras sin læresetning for å regne ut lengde, og 
	//	triangulerer deretter med differansen mellom oss og fiende.
	//
	void AimAngle(AimBoneType dst, float *NewAngle)
	{
		// I CS:S er Pitch vinkel fra bakke 89° til himmel - 89°. Yaw er fra 
		// -0.01 til -179.99 på den negative siden og fortsetter over til 
		// 179.94 ned til 0.06 på den andre siden. X er langs 180/-180 og Y 
		// er langs 90/-90.
		float src[3] = {
			MyPos[0] + MyEyesPos[0],
			MyPos[1] + MyEyesPos[1],
			MyPos[2] + MyEyesPos[2]
		};

		// Regn ut hvor langt det er fra oss til fiende (B - A).
		float dv[3] = { dst.X - src[0], dst.Y - src[1], dst.Z - src[2] };

		// Bruk pytagoras teorem for å finne hypotenusen/lengden til fienden. 
		// Denne er selvfølgelig lik for både pitch og yaw når Z er tatt med.
		float Hyp = sqrtf(dv[0] * dv[0] + dv[1] * dv[1] + dv[2] * dv[2]);

		// Z reverseres for å passe CS:S sin bruk av denne aksen.
		float Pitch = asinf(-dv[2] / Hyp) * (180.0f / (float)PI);

		float Yaw = atan2f(dv[1], dv[0]) * (180.0f / (float)PI);
		// Yaw += (dv[0] <= 0 ? 180.0f : 0); < trengs ved bruk av atanf() 
		// men var ikke tilstrekkelig i alle vinkler. atan2f() finner 
		// selv ut av negative/positive kvadranter for oss.

		// Oppdater siktevinkel til fiende.
		// Trekk fra rekylverdier spillet kommer til å legge til.
		// 2.0f er vanligvis weapon_recoil_scale i spillet, kan sjekkes via konsoll.
		NewAngle[0] = Pitch - PunchAngle[0] * 2.0f;
		NewAngle[1] = Yaw - PunchAngle[1] * 2.0f;
	}

} MyPlayer;




struct Players
{
	uintptr_t PlayerPtr;
	uintptr_t BonesPtr;
	int pNum = -1;
	int Team;
	int Health;
	bool Dormant;
	unsigned char LifeState;
	float EnemyPos[3];
	float AimAngle[2];
	AimBoneType AimBone;	
	uintptr_t dwClientStatePtr;
	uintptr_t PlayerInfoPtr;
	int EntityIndex;

	// Tar imot offsets og oppdaterer info per spiller.
	bool UpdateInformation(int n)
	{
		try {
			// Statisk peker til spillerbase.
			readmem(hProc.ClientModulePtr + dwEntityList + (n * 0x10), &PlayerPtr, sizeof(uintptr_t));	

			// Spillerindeks.
			readmem(PlayerPtr + 0x64, &EntityIndex, sizeof(int));

			// Prøv å les posisjonen til bein vi vil sikte på.
			readmem(PlayerPtr + m_dwBoneMatrix, &BonesPtr, sizeof(uintptr_t));
			readmem(BonesPtr + 0x30 * 8 + 0xC, &AimBone, sizeof(AimBone));

			// Hvilket lag er vi på?
			readmem(PlayerPtr + m_iTeamNum, &Team, sizeof(int));

			// Hvor mye HP har vi?
			readmem(PlayerPtr + m_iHealth, &Health, sizeof(int));

			// Lever vi?
			readmem(PlayerPtr + m_lifeState, &LifeState, sizeof(unsigned char));

			// Er vi dormant?
			readmem(PlayerPtr + m_bDormant, &Dormant, sizeof(bool));

			// Hva er XYZ posisjonen vår?
			readmem(PlayerPtr + m_vecOrigin, EnemyPos, sizeof(float)*3);

			// dwClientState
			readmem(hProc.EngineModulePtr + dwClientState, &dwClientStatePtr, sizeof(uintptr_t));

			// Hvor er base for alle spillernavn?
			readmem(dwClientStatePtr + dwClientState_PlayerInfo, &PlayerInfoPtr, sizeof(uintptr_t));

			if (Health == 0 && Team == 0)
				return false;

			pNum = n;
			return true;
		}
		catch (...) { 
			GetError(
				L"Players.UpdateInformation() Error. " + 
				std::to_wstring(pNum) + 
				L"Exiting."
			); 
			exit(EXIT_FAILURE); 
		}
	}

	//
	//	Funksjon for å hente spillernavn.
	//
	std::wstring GetName()
	{		
		uintptr_t pinfo_offset;
		uint8_t name[128] = {0};

		readmem(PlayerInfoPtr + 0x40, &pinfo_offset, sizeof(uintptr_t));
		readmem(pinfo_offset + 0xC, &pinfo_offset, sizeof(uintptr_t));
		readmem(pinfo_offset + 0x28 + ((EntityIndex-1) * 0x34), &pinfo_offset, sizeof(uintptr_t));
		readmem(pinfo_offset + 0x10, &name, sizeof(uint8_t)*128);

		std::string retval = (const char*)name;
		std::wstring wretval(retval.begin(), retval.end());
		return wretval;
	}

} OtherPlayers[64];




//
// Verden-til-Skjerm funksjon som tar en 3D visningsposisjon i form av en 
// 4x4 vektormatrise og konverterer til 2D koordinater som vi kan tegne på 
// 2D radar. 
// 
// *from er "view matrix" minnepeker i spillet, 
// *to er resulterende 2D XY koordinater som kan tegnes. 
//
bool WorldToScreen(float *from, float *to)
{
	float w = 0.0f;

	to[0] = MyPlayer.WorldToScreenMatrix.flMatrix[0][0] * from[0] +
		MyPlayer.WorldToScreenMatrix.flMatrix[0][1] * from[1] +
		MyPlayer.WorldToScreenMatrix.flMatrix[0][2] * from[2] +
		MyPlayer.WorldToScreenMatrix.flMatrix[0][3];

	to[1] = MyPlayer.WorldToScreenMatrix.flMatrix[1][0] * from[0] +
		MyPlayer.WorldToScreenMatrix.flMatrix[1][1] * from[1] +
		MyPlayer.WorldToScreenMatrix.flMatrix[1][2] * from[2] +
		MyPlayer.WorldToScreenMatrix.flMatrix[1][3];

	w = MyPlayer.WorldToScreenMatrix.flMatrix[3][0] * from[0] +
		MyPlayer.WorldToScreenMatrix.flMatrix[3][1] * from[1] +
		MyPlayer.WorldToScreenMatrix.flMatrix[3][2] * from[2] +
		MyPlayer.WorldToScreenMatrix.flMatrix[3][3];

	// Fant ingen mål innenfor visningsmatrisen.
	if (w < 0.01f)
		return false;
	
	// Fant mål, oppdaterer x,y for visning i 2D.
	float invw = 1.0f / w;
	to[0] *= invw;
	to[1] *= invw;

	int width = (int)(GameClientRect.right - GameClientRect.left);
	int height = (int)(GameClientRect.bottom - GameClientRect.top);

	float x = (float)(width / 2);
	float y = (float)(height / 2);

	x += 0.5f * to[0] * width + 0.5f;
	y -= 0.5f * to[1] * height + 0.5f;

	to[0] = x + GameClientRect.left;
	to[1] = y + GameClientRect.top;

	return true;
}




//
//	Pytagoras teorem hjelper oss å finne distansen til objekter.
//
float Get3dDistance(float* myCoords, float* enemyCoords)
{	
	float tmp[3] = {
		enemyCoords[0] - myCoords[0],
		enemyCoords[1] - myCoords[1],
		enemyCoords[2] - myCoords[2]
	};

	return sqrtf(
		(tmp[0] * tmp[0]) + 
		(tmp[1] * tmp[1]) + 
		(tmp[2] * tmp[2])
	);
}




//
//	Funksjon for å tagge alle fiender med hvor nærme siktet de er.
//	PitchOrYaw (1 = Pitch, 2 = Yaw).
//
float GetAimProximity(AimBoneType Target, int PitchOrYaw) 
{
	// I CS:S er Pitch vinkel fra bakke 89° til himmel - 89°. Yaw er fra 
	// -0.01 til -179.99 på den negative siden og fortsetter over til 
	// 179.94 ned til 0.06 på den andre siden. X er langs 180/-180 og Y 
	// er langs 90/-90.
	float src[3] = {
		MyPlayer.MyPos[0] + MyPlayer.MyEyesPos[0],
		MyPlayer.MyPos[1] + MyPlayer.MyEyesPos[1],
		MyPlayer.MyPos[2] + MyPlayer.MyEyesPos[2]
	};

	// Regn ut hvor langt det er fra oss til fiende (B - A).
	float dv[3] = { Target.X - src[0], Target.Y - src[1], Target.Z - src[2] };

	// Bruk pytagoras teorem for å finne hypotenusen/lengden til fienden. 
	// Denne er selvfølgelig lik for både pitch og yaw når Z er tatt med.
	float Hyp = sqrtf(dv[0] * dv[0] + dv[1] * dv[1] + dv[2] * dv[2]);
	float Pitch = asinf(-dv[2] / Hyp) * (180.0f / (float)PI);
	float Yaw = atan2f(dv[1], dv[0]) * (180.0f / (float)PI);

	// Returner forskjell i nåværende vinkel og mål etter ønske (Pitch/Yaw).
	float retval = 0;

	if (PitchOrYaw == 1)
		retval = Pitch - MyPlayer.Angle[0];
	else if (PitchOrYaw == 2)
		retval = Yaw - MyPlayer.Angle[1];

	if (retval < 0)
		retval = retval - (retval * 2);
	
	return retval;
}




//
//	Funksjon for å sjekke om mål er innefor angitte graders synsfelt.
//
bool TargetIsInCone(AimBoneType Target) {

	// Sjekk om både pitch og yaw er under 9° hver.
	if (GetAimProximity(Target,1) <= 9 && GetAimProximity(Target,2) <= 9)
		return true;
	else
		return false;
}




//
//	Fyres av via museklikk sjekk i hovedloop.
//  Sjekker og sikter på den nærmeste først.
//
void AimAtTarget()
{
	AimRegistry AR[64];
	bool PlayerAdded = false;

	// Legg spillere til en liste som sorteres i sanntid.
	for (int n=0; n<MAX_PLAYER_COUNT; n++) {

		// Gyldig spiller?
		if (OtherPlayers[n].pNum == -1)
			continue;

		// Ikke sikt på eget lag.
		if (MyPlayer.Team == OtherPlayers[n].Team)
			continue;

		// Ikke sikt på døde eller langt borte.
		if (
			OtherPlayers[n].LifeState > 0 || 
			OtherPlayers[n].Dormant
		) 
			continue;

		// Ikke sikt på ugyldig data.
		if (
			OtherPlayers[n].AimAngle[0] == 0.0f && 
			OtherPlayers[n].AimAngle[1] == 0.0f
		) 
			continue;

		// Ikke sikt på folk utenfor synsvinkel.
		if (!TargetIsInCone(OtherPlayers[n].AimBone))
			continue;

		// Legg til på liste.
		AR[n].AimProximity = GetAimProximity(OtherPlayers[n].AimBone,1) + 
							GetAimProximity(OtherPlayers[n].AimBone,2);
		AR[n].PlayerNum = OtherPlayers[n].pNum;
		AR[n].Distance = Get3dDistance(MyPlayer.MyEyesPos, OtherPlayers[n].EnemyPos);

		PlayerAdded = true;
	}

	if (!PlayerAdded)
		return;

	// Sorter (sjekk struct).
	std::sort(AR, AR+64);

	// Sikt...
	MyPlayer.AimAtEnemy(OtherPlayers[AR[0].PlayerNum].AimAngle);
}




void ProjectText(
	int Size, 
	bool Bold, 
	bool Italic, 
	bool Underline, 
	const char* Font, 
	int x, int y, 
	COLORREF c, 
	std::wstring s, 
	HDC* dc, 
	int Alignment, 
	bool Transparent
){
	HFONT font = CreateFontA(
		Size,			// Høyde.
		0,			// Bredde.
		0, 
		0, 
		(Bold?700:400),		// 0-1000. 400=Normal, 700=bold.
		Italic,			// Italic.
		Underline,		// Underline.
		FALSE,			// Strikeout.
		DEFAULT_CHARSET,	// Charset.
		OUT_OUTLINE_PRECIS,	
		CLIP_DEFAULT_PRECIS, 
		ANTIALIASED_QUALITY,	// Kvalitet.
		VARIABLE_PITCH,	
		(LPCSTR)Font		// Font.
	);
	
	SetTextAlign(*dc, Alignment | TA_NOUPDATECP);
	SetTextColor(*dc, c);
	SelectObject(*dc, font);
	SetBkColor(*dc, RGB(32,32,32));
	SetBkMode(*dc, (Transparent? TRANSPARENT : OPAQUE));
	TextOutW(*dc, x, y, s.c_str(), s.length());
	DeleteObject(font);
}




void EspText(int x, int y, COLORREF c, std::wstring s, HDC* dc, int Alignment)
{
	// Tilpass font for ESP.
	HFONT font = CreateFontW(
		14,			// Høyde.
		0,			// Bredde.
		0,
		0,
		700,			// 0-1000. 400=Normal, 700=bold.
		FALSE,			// Italic.
		FALSE,			// Underline.
		FALSE,			// Strikeout.
		DEFAULT_CHARSET,	// Charset.
		OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,	// Kvalitet.
		VARIABLE_PITCH,
		TEXT("Verdana")		// Font.
	);

	SetTextAlign(*dc, Alignment | TA_NOUPDATECP);
	SetTextColor(*dc, c);
	SelectObject(*dc, font);
	SetBkColor(*dc, Black);
	SetBkMode(*dc, TRANSPARENT);
	TextOutW(*dc, x, y, s.c_str(), s.length());
	DeleteObject(font);
}




//
//	Oppdaterer projisering.
//
void UpdateProjection()
{
	//
	//	Hent kontekstkoordinater rett fra spillvindu.
	//	HUSK: GetClientRect() = Vinduoppløsning. 
	//	      GetWindowRect() = Vinduplassering.
	//	
	//	Derfor bruker vi client til å oppdatere bakgrunn med.
	//
	GetClientRect(hProc.__HWNDCss, &GameClientRect);
	int cx = GameClientRect.right;
	int cy = GameClientRect.bottom;

	//
	//	Opprett mellomlager for tegning.
	//	Vi lar front-HDC være i fred mens vi tegner på et mellomlager
	//	og flipper over dette når vi er ferdig - kun én oppdatering. 
	//
	BufferDC = CreateCompatibleDC(hdc);
	HBITMAP BufferBitmap = CreateCompatibleBitmap(hdc, cx, cy);
	SelectObject(BufferDC, BufferBitmap);

	//
	//	Overskriv gjeldende bakgrunn for å unngå artifakter fra 
	//	forrige bilde. Vi oppnår gj.siktighet med samme farge 
	//	vi brukte som parameter i SetLayeredWindowAttributes (WinMain).
	//
	HBRUSH BgBrush = CreateSolidBrush(TrueWhite);
	FillRect(BufferDC, &GameClientRect, BgBrush);
	DeleteObject(BgBrush);
	
	//
	//	Never fear, I is here.
	// 
	ProjectText(
		15,true,false,false,"Arial",200,30,CYAN,
		L"CS:GO AimRadar by ժʝ_ is running.",
		&BufferDC,TA_CENTER,false
	);
	
	//
	//	Tegn fiender som befinner seg i verden-til-skjerm.
	//	Har aldri helt skjønt greia med store stygge bokser så jeg 
	//	velger å bruke en liten rød [X] og avstandstekst i stedet.
	//
	try {
	
	float RadarXY[2];
	float SourceXYZ[3];

	for (int n=0; n<MAX_PLAYER_COUNT; n++) {

		//
		//	Avslutt ved enden av gyldige spillere.
		//
		if (OtherPlayers[n].pNum == -1)
			continue;
		
		//
		//	Hopp over døde og folk på eget lag etc.
		//
		if (
			OtherPlayers[n].LifeState > 0 || 
			OtherPlayers[n].Team == MyPlayer.Team || 
			OtherPlayers[n].Dormant
		)
			continue;
		
		//
		//	Visningsmatrisen hjelper å koordinere ESP, men er noe 
		//	jeg vil prøve å få til med bruk av vinkler senere.
		//
		SourceXYZ[0] = OtherPlayers[n].AimBone.X;
		SourceXYZ[1] = OtherPlayers[n].AimBone.Y;
		SourceXYZ[2] = OtherPlayers[n].AimBone.Z;

		if (WorldToScreen(SourceXYZ, RadarXY)) {
			EspText(
				(int)RadarXY[0] - GameClientRect.left,
				(int)RadarXY[1] - GameClientRect.top,
				Red,
				OtherPlayers[n].GetName() +
				L" Distance:"+ 
				std::to_wstring(
					(int)Get3dDistance(
						MyPlayer.MyPos, 
						OtherPlayers[n].EnemyPos
					)
				).substr(0,6) +
				L" HP:"+
				std::to_wstring(OtherPlayers[n].Health),
				&BufferDC,
				TA_CENTER
			);
		}

	}} catch (...) { 
		GetError(L"Error during drawing to 2D radar/ESP. Exiting."); 
		exit(EXIT_FAILURE); 
	}


	//
	//	FEILSØKING - DEBUG INFO.
	//
	if (AntallSpillereFunnet > 1 && !MyPlayer.Dormant && MyPlayer.LifeState == 0) {

		ProjectText(12, false, false, false, "Verdana", 10, 30, White, L"Health: "+ std::to_wstring(MyPlayer.Health), &BufferDC, TA_LEFT, false);
		ProjectText(12, false, false, false, "Verdana", 10, 30+12, White, L"Team: " + std::to_wstring(MyPlayer.Team), &BufferDC, TA_LEFT, false);
		ProjectText(12, false, false, false, "Verdana", 10, 30+2*12, White, L"Player XYZ: " +
			std::to_wstring(MyPlayer.MyPos[0]).substr(0,6) + L" " + 
			std::to_wstring(MyPlayer.MyPos[1]).substr(0,6) + L" " + 
			std::to_wstring(MyPlayer.MyPos[2]).substr(0,6), 
			&BufferDC, 
			TA_LEFT,
			false
		);
		ProjectText(13, false, false, false, "Verdana", 10, 30+3*12, White, L"Number of players: " + std::to_wstring(AntallSpillereFunnet), &BufferDC, TA_LEFT,false);
		ProjectText(12, false, false, false, "Verdana", 10, 30+4*12, White, L"m_vecViewOffset: "+ 
			std::to_wstring(MyPlayer.MyPos[0]).substr(0,6) + L" " + 
			std::to_wstring(MyPlayer.MyPos[1]).substr(0,6) + L" " + 
			std::to_wstring(MyPlayer.MyPos[2]).substr(0,6),
			&BufferDC, 
			TA_LEFT, 
			false
		);
		ProjectText(12, false, false, false, "Verdana", 10, 30+5*12, White, L"dwClientState_ViewAngles: "+ 
			std::to_wstring(MyPlayer.Angle[0]).substr(0,6) + L" " + 
			std::to_wstring(MyPlayer.Angle[1]).substr(0,6),
			&BufferDC, 
			TA_LEFT, 
			false
		);
	}
	
	int textposY = 250;
	int ESPCounter = 0;

	// Spillerliste (Rød = T, Blå = CT).
	for (int n = 1; n<= AntallSpillereFunnet; n++) {
		if (
			OtherPlayers[n].Team == MyPlayer.Team || 
			OtherPlayers[n].Team == 0 || 
			OtherPlayers[n].Health == 0 || 
			OtherPlayers[n].Dormant
		) 
			continue;

		textposY += 12;
		ESPCounter += 1;
		ProjectText(12, false, false, false, "Verdana", 10, textposY, 
			(OtherPlayers[n].Team == 2 ? Red : CTBlue), 
			(OtherPlayers[n].Team == 2 ? L"Terr" : L"CT") + std::to_wstring(ESPCounter) + 
			L" HP:"+ std::to_wstring(OtherPlayers[n].Health) +
			L" °AimYaw:"+ std::to_wstring(GetAimProximity(OtherPlayers[n].AimBone, 2)).substr(0,4) + 
			L" °AimPitch:"+ std::to_wstring(GetAimProximity(OtherPlayers[n].AimBone, 1)).substr(0,4) + 
			L" Within Aim Cone:"+ std::to_wstring(TargetIsInCone(OtherPlayers[n].AimBone)) + 
			L" AimBone:(" + 
				std::to_wstring((int)OtherPlayers[n].AimBone.X) +L" "+
				std::to_wstring((int)OtherPlayers[n].AimBone.Y) +L" "+
				std::to_wstring((int)OtherPlayers[n].AimBone.Z) +L")",
			&BufferDC,
			TA_LEFT,
			true
		); 
	}

	//
	//	Skriv mellomlager til prosjektoren og rydd opp.
	//
	BitBlt(hdc, 0, 0, cx, cy, BufferDC, 0, 0, SRCCOPY);
	DeleteObject(BufferBitmap);
	DeleteDC(BufferDC);
}




//
//	Oppdater alle samlede minnedata.
//
void UpdateDataCollection()
{
	// Oppdater spillerdata (spilleren).
	MyPlayer.UpdateInformation();

	// Oppdater spillerdata (andre).
	AntallSpillereFunnet = 1; // 1=spilleren.
	std::fill(OtherPlayers, OtherPlayers + MAX_PLAYER_COUNT, Players{});

	// Forfrisk spillerdata.
	for (int n=0; n<MAX_PLAYER_COUNT; n++) {
		if(!OtherPlayers[n].UpdateInformation(n))
			continue;
		
		MyPlayer.AimAngle( 
			OtherPlayers[n].AimBone, 
			OtherPlayers[n].AimAngle
		);

		AntallSpillereFunnet += 1;
	}
}




void UpdateLayerStatus() 
{
	// Lukk programmet hvis spillet ikke kjører.
	if (FindWindowW(NULL, L"Counter-Strike: Global Offensive - Direct3D 9") == NULL) {
		//GetError(L"Can't find the game, closing.");
		exit(EXIT_SUCCESS);
	}
	
	// Fant spill, hent RECT attributter.
	GetWindowRect(hProc.__HWNDCss, &GameWindowRect);

	// Sett tilsvarende høyde, bredde og posisjon på prosjektor.
	SetWindowPos(
		*ProjectorHwnd, 
		HWND_TOPMOST, 
		GameWindowRect.left,
		GameWindowRect.top,
		GameWindowRect.right - GameWindowRect.left,
		GameWindowRect.bottom - GameWindowRect.top,
		SWP_SHOWWINDOW
	);
}