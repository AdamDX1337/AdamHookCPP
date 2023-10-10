// Thank Mastoid

#pragma once
#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment(lib, "MinHook.x64.lib")

#include <Windows.h>
#include <iostream>
#include <string>
#include "d3d9.h"
#include "d3dx9.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"
#include "MinHook.h"
#include "sdk.h"
#include <TlHelp32.h>
#include <tchar.h>
#include <vector>
#include <stdlib.h>
// BaseModule IGNORE
DWORD GetModuleBaseAddress(TCHAR* lpszModuleName, DWORD pID) {
	DWORD dwModuleBaseAddress = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID); // make snapshot of all modules within process
	MODULEENTRY32 ModuleEntry32 = { 0 };
	ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(hSnapshot, &ModuleEntry32)) //store first Module in ModuleEntry32
	{
		do {
			if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0) // if Found Module matches Module we look for -> done!
			{
				dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
				break;
			}
		} while (Module32Next(hSnapshot, &ModuleEntry32)); // go through Module entries in Snapshot and store in ModuleEntry32


	}
	CloseHandle(hSnapshot);
	return dwModuleBaseAddress;
}
//Pointer Scan IGNORE
DWORD GetPointerAddress(HWND hwnd, DWORD gameBaseAddr, DWORD address, std::vector<DWORD> offsets)
{
	DWORD pID = NULL; // Game process ID
	GetWindowThreadProcessId(hwnd, &pID);
	HANDLE phandle = NULL;
	phandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
	if (phandle == INVALID_HANDLE_VALUE || phandle == NULL);

	DWORD offset_null = NULL;
	ReadProcessMemory(phandle, (LPVOID*)(gameBaseAddr + address), &offset_null, sizeof(offset_null), 0);
	DWORD pointeraddress = offset_null; // the address we need
	for (int i = 0; i < offsets.size() - 1; i++) // we dont want to change the last offset value so we do -1
	{
		ReadProcessMemory(phandle, (LPVOID*)(pointeraddress + offsets.at(i)), &pointeraddress, sizeof(pointeraddress), 0);
	}
	return pointeraddress += offsets.at(offsets.size() - 1); // adding the last offset
}



namespace offset {
	constexpr std::ptrdiff_t tagswitch = 0x25E9D00;
}


//HOOKS
typedef HRESULT(_stdcall* EndScene)(LPDIRECT3DDEVICE9 pDevice);
EndScene EndSceneHook;
EndScene EndSceneTramp;

typedef bool(__fastcall* CSessionPost)(void* pThis, CCommand* pCommand, bool ForceSend);
CSessionPost CSessionPostHook;
CSessionPost CSessionPostTramp;

typedef CAddPlayerCommand* (__fastcall* GetCAddPlayerCommand)(void* pThis, CString* User, CString* Name, int nMachineId, bool bHotjoin, __int64 a6);
GetCAddPlayerCommand CAddPlayerCommandHook;
GetCAddPlayerCommand CAddPlayerCommandTramp;

typedef __int64(__fastcall* CGameStateSetPlayer)(void* pThis, int* Tag);
CGameStateSetPlayer CGameStateSetPlayerHook;
CGameStateSetPlayer CGameStateSetPlayerTramp;

typedef void(__fastcall* SetDebugTooltipsEnabled)(COption* pThis, bool bSelected);
SetDebugTooltipsEnabled SetDebugTooltipsEnabledHook;
SetDebugTooltipsEnabled SetDebugTooltipsEnabledTramp;

//NORMAL FUNCTION CALLS
typedef CStartGameCommand* (__fastcall* GetCStartGameCommand)(void* pThis);
GetCStartGameCommand CStartGameCommandFunc;

typedef CSetDLCsCommand* (__fastcall* GetCSetDLCsCommand)(void* pThis, unsigned int nDLCs);
GetCSetDLCsCommand CSetDLCsCommandFunc;

typedef LPVOID(__fastcall* GetCCommand)(__int64 a1);
GetCCommand GetCCommandFunc;

//CLASS POINTERS
void* pCSession = nullptr;
void* pCGameState = nullptr;
COption* pSetDebugTooltipsEnabled = nullptr;

//DRAWING VARS
int KeyArray[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
char TagBuffer[8];
bool bDebugOutputEnabled = true;
bool bMaxNameSize = false;
bool bJoinAsGhost = false;
bool bEnabletdebug = false;
bool bMenuOpen = true;

//RANDOM VARS
int iMyMachineID = 0;
int iMachineIDFake = 50;
__int64 iParadoxSocialID = 0;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
IDirect3D9* g_pD3D = NULL;
HWND window = NULL;
uintptr_t GameBase = (uintptr_t)GetModuleHandleA("hoi4.exe");


//Memory Addresses
DWORD tagswitchAddr = 0x25E9D00;
std::vector<DWORD> tagSwitchOffset{ 0xE40 }; 
DWORD TSPtrAddr = GetPointerAddress(window, GameBase, tagswitchAddr, tagSwitchOffset);


void printm(std::string str)
{
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);

	std::cout << "[StinkyHook] " << str << std::endl;
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcID;
	GetWindowThreadProcessId(handle, &wndProcID);

	if (GetCurrentProcessId() != wndProcID)
	{
		return TRUE;
	}

	window = handle;
	return FALSE;
}

HWND GetProcessWindow()
{
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}

bool GetD3D9Device(void** pTable, size_t size)
{
	if (!pTable)
	{
		return false;
	}

	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!g_pD3D)
	{
		return false;
	}

	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetProcessWindow();
	d3dpp.Windowed = true;

	g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);

	if (!g_pd3dDevice)
	{
		g_pD3D->Release();

		return false;
	}

	memcpy(pTable, *reinterpret_cast<void***>(g_pd3dDevice), size);

	g_pd3dDevice->Release();
	g_pD3D->Release();

	return true;
}

HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	static bool init = false;

	if (!init)
	{
		printm("EndScene hooked.");
		ImGui::CreateContext();
		ImGui::StyleColorsLight();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX9_Init(pDevice);

		init = true;
	}

	if (GetAsyncKeyState(VK_INSERT) & 1)
		bMenuOpen = !bMenuOpen;

	if (bMenuOpen)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.WantCaptureMouse = true;
		io.WantCaptureKeyboard = true;

		if (GetAsyncKeyState(VK_LBUTTON))
		{
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
		}
		else
		{
			io.MouseReleased[0] = true;
			io.MouseDown[0] = false;
			io.MouseClicked[0] = false;
		}

		for (int i : KeyArray)
		{
			if (GetAsyncKeyState(i) & 1)
			{
				io.AddInputCharacter(i);
			}
		}

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowSize(ImVec2(300, 465));
		ImGui::Begin("StinkyHook", &bMenuOpen);
		ImGui::Text("Cheat Settings");
		ImGui::Checkbox("Enable Debug Output", &bDebugOutputEnabled);
		ImGui::Text("");
		ImGui::Text("Function Calls");



		if (ImGui::Button("Toggle debug", ImVec2(140, 28)) && pSetDebugTooltipsEnabled != nullptr)
		{
			bEnabletdebug = !bEnabletdebug;

			SetDebugTooltipsEnabledTramp(pSetDebugTooltipsEnabled, bEnabletdebug);
		}



		if (ImGui::Button("Start Game", ImVec2(140, 28)) && pCSession != nullptr)
		{
			CStartGameCommand* StartGame = (CStartGameCommand*)GetCCommandFunc(40);

			StartGame = CStartGameCommandFunc(StartGame);

			CSessionPostTramp(pCSession, StartGame, true);
		}

		if (ImGui::Button("DOS Host", ImVec2(140, 28)))
		{
		}

		ImGui::Text("");
		ImGui::Text("Tag Switching");
		ImGui::SetNextItemWidth(70.f);
		ImGui::InputText("", TagBuffer, IM_ARRAYSIZE(TagBuffer));
		ImGui::SameLine();
		if (ImGui::Button("Tag Switch") && pCGameState != nullptr)
		{
			std::string sTagBuffer = TagBuffer;

			if (sTagBuffer.length() > 0)
			{
				int iNewTag = std::stoi(sTagBuffer);
				int* pNewTag = &iNewTag;


				CGameStateSetPlayerTramp(pCGameState, pNewTag);
				//WriteProcessMemory(GetProcessWindow(), (LPVOID*)(TSPtrAddr), pNewTag, 4, 0);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			memset(TagBuffer, 0, sizeof(TagBuffer));
		}
		ImGui::Text("");
		ImGui::Text("Credits: AdamDX");

		ImGui::End();

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}

	return EndSceneTramp(pDevice);
}

bool __fastcall hkCSessionPost(void* pThis, CCommand* pCommand, bool ForceSend)
{
	if (bDebugOutputEnabled)
		printm("CSession::Post() called.");

	pCSession = pThis;

	return CSessionPostTramp(pThis, pCommand, ForceSend);
}

CAddPlayerCommand* __fastcall hkCAddPlayerCommand(void* pThis, CString* User, CString* Name, int nMachineId, bool bHotjoin, __int64 a6)
{
	if (bDebugOutputEnabled)
		printm("CAddPlayerCommand::CAddPlayerCommand called.");

	if (iParadoxSocialID == 0)
		iParadoxSocialID = a6;

	iMachineIDFake = 50;
	iMyMachineID = nMachineId;

	if (bMaxNameSize)
	{
		Name->_str = "";

		for (int i = 0; i < 1000; i++)
		{
			Name->_str = Name->_str + "WWWWWWWWWWWW";
		}
	}

	if (bJoinAsGhost)
	{
		User->_str = "";
		Name->_str = "";
	}

	return CAddPlayerCommandTramp(pThis, User, Name, nMachineId, bHotjoin, a6);
}

__int64 __fastcall hkCGameStateSetPlayer(void* pThis, int* Tag)
{
	if (bDebugOutputEnabled)
		printm("CGameState::SetPlayer called.");

	pCGameState = pThis;

	return CGameStateSetPlayerTramp(pThis, Tag);
}

void __fastcall hkSetDebugTooltipsEnabled(COption* pThis, bool bSelected)
{
	if (bDebugOutputEnabled)
		printm("CMapModeManager::SetDebugTooltipsEnabled called.");

	pSetDebugTooltipsEnabled = pThis;

	SetDebugTooltipsEnabledTramp(pThis, bEnabletdebug);
}

void HookFunctions()
{
	void* d3d9Device[119];

	if (GetD3D9Device(d3d9Device, sizeof(d3d9Device)))
	{
		EndSceneHook = EndScene((uintptr_t)d3d9Device[42]);

		MH_CreateHook(EndSceneHook, &hkEndScene, (LPVOID*)&EndSceneTramp);
		MH_EnableHook(EndSceneHook);
	}

	CSessionPostHook = CSessionPost(GameBase + 0x13A48D0); //0xC1A4C0 1.7.1 //0x13A48D0 modern

	MH_CreateHook(CSessionPostHook, &hkCSessionPost, (LPVOID*)&CSessionPostTramp);
	MH_EnableHook(CSessionPostHook);

	CAddPlayerCommandHook = GetCAddPlayerCommand(GameBase + 0xF85030); //0x946F50 1.7.1 //0xF85030 modern

	MH_CreateHook(CAddPlayerCommandHook, &hkCAddPlayerCommand, (LPVOID*)&CAddPlayerCommandTramp);
	MH_EnableHook(CAddPlayerCommandHook);

	CGameStateSetPlayerHook = CGameStateSetPlayer(GameBase + 0x25E9D00); //0xB8D50 1.7.1 //0x142090 modern //0x25E9D00 newest

	MH_CreateHook(CGameStateSetPlayerHook, &hkCGameStateSetPlayer, (LPVOID*)&CGameStateSetPlayerTramp);
	MH_EnableHook(CGameStateSetPlayerHook);

	SetDebugTooltipsEnabledHook = SetDebugTooltipsEnabled(GameBase + 0x25E9829); //0x4D7B10 1.7.1 //0x8B69F0 modern // NEWEST 0x25E9829

	MH_CreateHook(SetDebugTooltipsEnabledHook, &hkSetDebugTooltipsEnabled, (LPVOID*)&SetDebugTooltipsEnabledTramp);
	MH_EnableHook(SetDebugTooltipsEnabledHook);

	CSetDLCsCommandFunc = GetCSetDLCsCommand(GameBase + 0xF851D0); //0x947110 1.7.1 //0xF851D0 modern
	CStartGameCommandFunc = GetCStartGameCommand(GameBase + 0xEBD230); //0x831910 1.7.1 //0xEBD230 modern
	GetCCommandFunc = GetCCommand(GameBase + 0x16EC008); //0x16EC008 modern
}

DWORD WINAPI MainThread(PVOID base)
{
	HookFunctions();
	FreeLibraryAndExitThread(static_cast<HMODULE>(base), 1);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		MH_Initialize();
		CreateThread(nullptr, NULL, MainThread, hModule, NULL, nullptr);
		AllocConsole();
		printm("Cheat loaded.");
	}

	return TRUE;
}
