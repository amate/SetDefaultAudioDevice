// SetDefaultAudioDevice.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <vector>
#include <regex>
#include <Windows.h>
#include <ObjBase.h>
#include <Mmsystem.h>
#include <devpropdef.h>
#include <atlbase.h>
#include <atlstr.h>
#include "PolicyConfig.h"

#pragma comment(lib, "Winmm.lib")

using namespace std;

#define SAFE_RELEASE(x) if (x) { x->Release(); x = 0; }

bool SetClipboardText(const CString &str, HWND hWnd)
{
	if ( str.IsEmpty() )
		return false;

	int 	nByte = (str.GetLength() + 1) * sizeof(TCHAR);
	HGLOBAL hText = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, nByte);
	if (hText == NULL)
		return false;

	BYTE*	pText = (BYTE *) ::GlobalLock(hText);

	if (pText == NULL)
		return false;

	::memcpy(pText, (LPCTSTR) str, nByte);

	::GlobalUnlock(hText);

	::OpenClipboard(hWnd);
	::EmptyClipboard();
	::SetClipboardData(CF_UNICODETEXT, hText);
	::CloseClipboard();
	return true;
}

struct EndPointData {
	CString	devID;
	CString name;
	bool	bDefault;

	EndPointData() : bDefault(false) { }
};

// See: http://social.microsoft.com/Forums/en/Offtopic/thread/9ebd7ad6-a460-4a28-9de9-2af63fd4a13e
HRESULT RegisterDevice(LPCWSTR devID, ERole role)
{
	IPolicyConfig *pPolicyConfig = nullptr;

	HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig), (LPVOID *)&pPolicyConfig);
	if (pPolicyConfig == nullptr) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig10_1), (LPVOID *)&pPolicyConfig);
	}
	if (pPolicyConfig == nullptr) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig10), (LPVOID *)&pPolicyConfig);
	}
	if (pPolicyConfig == nullptr) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig7), (LPVOID *)&pPolicyConfig);
	}
	if (pPolicyConfig == nullptr) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
	}

	if (pPolicyConfig != NULL) {
		hr = pPolicyConfig->SetDefaultEndpoint(devID, role);
		SAFE_RELEASE(pPolicyConfig);
	}

	return hr;
}

void ShowUsage()
{
	cout << "Usage: SetDefaultAudioDevice <deviceID> <role>\n";
	cout << "Where: <deviceID> is a GUID including braces\n";
	cout << "       <role> is either 'console', 'multimedia' or 'communications'.\n\n";
	cout << "Example: SetDefaultAudioDevice {24dfc80a-680f-4748-8627-c340cb14f187} multimedia\n\n";
	cout << "Thank-you to EreTIk and Jonny Best and everyone else on this forum post:\n";
	cout << "http://social.microsoft.com/Forums/en/Offtopic/thread/9ebd7ad6-a460-4a28-9de9-2af63fd4a13e\n\n";
}

// Windows Core Audio で EndPoint を取得
// http://blog.firefly-vj.net/2008/08/windows-core-audio-endpoint.html
bool	GetEndPointDeviceData(vector<EndPointData>& vecEndPoint)
{
	CComPtr<IMMDeviceEnumerator>	spEnumerator;
	if (FAILED(spEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator))))
		return false;

	CComPtr<IMMDeviceCollection>	spCollection;
	if (FAILED(spEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &spCollection)))
		return false;

	CString defaultDevID;
	CComPtr<IMMDevice>	spDefaultDevice;
	if (SUCCEEDED(spEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &spDefaultDevice))) {
		LPWSTR id;
		if (SUCCEEDED(spDefaultDevice->GetId(&id))) {
			defaultDevID = id;
			::CoTaskMemFree(id);
		}
	}

	UINT nCount = 0;
	spCollection->GetCount(&nCount);
	for (UINT i = 0; i < nCount; ++i) {
		CComPtr<IMMDevice>	spDevice;
		spCollection->Item(i, &spDevice);
		if (spDevice) {
			EndPointData	data;
			LPWSTR id;
			if (SUCCEEDED(spDevice->GetId(&id))) {
				data.devID = id;
				::CoTaskMemFree(id);
				if (data.devID == defaultDevID)
					data.bDefault = true;
			}
				
			CComPtr<IPropertyStore>	spProperty;
			spDevice->OpenPropertyStore(STGM_READ, &spProperty);
			if (spProperty) {
				PROPVARIANT	varName;
				PropVariantInit(&varName); 
				if (SUCCEEDED(spProperty->GetValue(PKEY_Device_FriendlyName, &varName)))
					data.name = varName.pwszVal;
				PropVariantClear(&varName);
			}
			vecEndPoint.push_back(data);	// 追加
		}
	}

	return true;
}

void Run()
{
	vector<EndPointData>	vecEndPoint;
	vecEndPoint.reserve(10);
	GetEndPointDeviceData(vecEndPoint);

	{
		std::wstring cmdLine = ::GetCommandLineW();
		if (cmdLine.find(L"-initAllSoundDevice") != std::wstring::npos) {
			for (auto& endPoint : vecEndPoint) {
				if (endPoint.bDefault == false) {
					RegisterDevice(endPoint.devID, eMultimedia);
					::Sleep(1000);
					::PlaySound(_T("mssound.wav"), NULL, SND_FILENAME | SND_SYNC);
					::Sleep(1000);
				}
			}
			for (auto& endPoint : vecEndPoint) {
				if (endPoint.bDefault == true) {
					RegisterDevice(endPoint.devID, eMultimedia);
					break;
				}
			}
			return ;
		}
	}

	// Ctrl を押しながら起動で　デバイス名をクリップボードへコピーする
	if (::GetKeyState(VK_CONTROL) < 0) {
		CString cliptext;
		for (auto it = vecEndPoint.cbegin(); it != vecEndPoint.cend(); ++it)
			cliptext.AppendFormat(_T("%s\r\n"), it->name);
		SetClipboardText(cliptext, NULL);
		return ;
	}

	// コマンドラインからのトグル操作
	std::wregex rx(L"-t\"([^\"]+)\" -t\"([^\"]+)\"");
	std::wsmatch result;
	std::wstring temp = ::GetCommandLineW();
	if (std::regex_search(temp, result, rx)) {
		CString toggleDev1 = result[1].str().c_str();
		CString toggleDev2 = result[2].str().c_str();
		int toggleDevIndex1 = -1;
		int toggleDevIndex2 = -1;
		for (int i = 0; i < static_cast<int>(vecEndPoint.size()); ++i) {
			if (vecEndPoint[i].name.Find(toggleDev1) != -1)
				toggleDevIndex1 = i;
			else if (vecEndPoint[i].name.Find(toggleDev2) != -1)
				toggleDevIndex2 = i;
		}
		if (toggleDevIndex1 != -1 && toggleDevIndex2 != -1) {
			if (vecEndPoint[toggleDevIndex1].bDefault) {	// 1 がデフォルトなら 2 に
				RegisterDevice(vecEndPoint[toggleDevIndex2].devID, eMultimedia);
			} else if (vecEndPoint[toggleDevIndex2].bDefault) {	// 2 がデフォルトなら 1 に
				RegisterDevice(vecEndPoint[toggleDevIndex1].devID, eMultimedia);
			} else {	// それ以外だったら 1 に
				RegisterDevice(vecEndPoint[toggleDevIndex1].devID, eMultimedia);
			}
			return ;
		}
	} else {
		std::wregex rx(L"-t\"([^\"]+)\"");
		std::wsmatch result;
		std::wstring temp = ::GetCommandLineW();
		if (std::regex_search(temp, result, rx)) {
			for (auto& endPoint : vecEndPoint) {
				if (endPoint.name.Find(result.str(1).c_str()) != -1) {
					RegisterDevice(endPoint.devID, eMultimedia);
					return ;
				}
			}
		}
	}

	if (vecEndPoint.size() > 0) {
		HMENU hMenu = ::CreatePopupMenu();
		int nID = 1;
		for (auto it = vecEndPoint.cbegin(); it != vecEndPoint.cend(); ++it) {
			::AppendMenu(hMenu, it->bDefault ? MF_CHECKED : 0, nID, it->name);
			++nID;
		}
		POINT pt;
		::GetCursorPos(&pt);
		HWND hWnd = CreateWindow(_T("STATIC"), _T(""), 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
		SetForegroundWindow(hWnd);
		int nCmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
		if (nCmd) {
			RegisterDevice(vecEndPoint[nCmd - 1].devID, eMultimedia);
		}
	}
}

//int _tmain(int argc, _TCHAR* argv[])
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CoInitialize(NULL);

	OleInitialize(NULL);

	Run();

	OleUninitialize();

	CoUninitialize();

	return 0;
#if 0
	if (argc != 3)
	{
		ShowUsage();
		return 0;
	}

	ERole role = eMultimedia;

	if (_wcsicmp(argv[2], L"console") == 0)
	{
		role = eConsole;
	}
	else if (_wcsicmp(argv[2], L"communications") == 0)
	{
		role = eCommunications;
	}
	else if (_wcsicmp(argv[2], L"multimedia") != 0)
	{
		ShowUsage();
		return 0;
	}

	CoInitialize(NULL);
	return RegisterDevice(argv[1], role);
#endif
}
