// SetDefaultAudioDevice.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <vector>
#include <Windows.h>
#include <ObjBase.h>
#include <devpropdef.h>
#include <atlbase.h>
#include <atlstr.h>
#include "PolicyConfig.h"

using namespace std;

#define SAFE_RELEASE(x) if (x) { x->Release(); x = 0; }

struct EndPointData {
	CString	devID;
	CString name;
	bool	bDefault;

	EndPointData() : bDefault(false) { }
};

// See: http://social.microsoft.com/Forums/en/Offtopic/thread/9ebd7ad6-a460-4a28-9de9-2af63fd4a13e
HRESULT RegisterDevice(LPCWSTR devID, ERole role)
{
	IPolicyConfig *pPolicyConfig;

	HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig), (LPVOID *)&pPolicyConfig);

	if (pPolicyConfig != NULL)
	{
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

// Windows Core Audio ‚Å EndPoint ‚ðŽæ“¾
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
			vecEndPoint.push_back(data);	// ’Ç‰Á
		}
	}

	return true;
}

//int _tmain(int argc, _TCHAR* argv[])
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CoInitialize(NULL);
	{
		vector<EndPointData>	vecEndPoint;
		vecEndPoint.reserve(10);
		GetEndPointDeviceData(vecEndPoint);

		if (vecEndPoint.size() > 0) {
			HMENU hMenu = ::CreatePopupMenu();
			int nID = 1;
			for (auto it = vecEndPoint.cbegin(); it != vecEndPoint.cend(); ++it) {
				::AppendMenu(hMenu, it->bDefault ? MF_CHECKED : 0, nID, it->name);
				++nID;
			}
			POINT pt;
			::GetCursorPos(&pt);
			HWND hWnd = CreateWindow(_T("STATIC"), _T(""), 0, 0,0, 0,0, NULL, NULL, GetModuleHandle(NULL), NULL);
			SetForegroundWindow(hWnd);
			int nCmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
			if (nCmd) {
				RegisterDevice(vecEndPoint[nCmd - 1].devID, eMultimedia);
			}
		}
	}
	CoUninitialize();
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
