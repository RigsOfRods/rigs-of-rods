#ifdef _WIN32
#include "Keys.h"
#include <algorithm>
#include <windows.h>
#include <SFML/System/Lock.hpp>

using namespace ckeys;

HHOOK hHook = NULL;  // windows

Keys* kk = nullptr;


///  Key hook
///-----------------------------------------------------------------------------
LRESULT CALLBACK KeyHandler(int nCode, WPARAM wp, LPARAM lp)
{
	if (nCode < 0)
		return CallNextHookEx(hHook, nCode, wp, lp);

	if (nCode == HC_ACTION && kk != nullptr)
	{
		KBDLLHOOKSTRUCT kh = *((KBDLLHOOKSTRUCT*)lp);
		int vk = kh.vkCode & 0xFF,
			sc = kh.scanCode & 0xFF,
			ext = kh.flags & LLKHF_EXTENDED > 0 ? 1 : 0;

		bool press = wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN;
		bool release = wp == WM_SYSKEYUP || wp == WM_KEYUP;

		//  pressed list update
		if (press || release)
		{
			KeyCode kc;
			kc.vk = vk;  kc.sc = sc;  kc.ext = ext;

			sf::Lock(kk->mutex);
			if (press)
			{
				auto& l = kk->keyCodes;
				if (std::find(l.begin(), l.end(), kc) == l.end())
					kk->keyCodes.push_back(kc);
			}else
				kk->keyCodes.remove(kc);
		}

		//  layout update
		if (!kk->keys.empty())
		{
			//****  vk map
			//  map numpad keys  //
			const static int vkN[VK_DOWN - VK_PRIOR +1] = {
				VK_NUMPAD9, VK_NUMPAD3, // VK_PRIOR VK_NEXT
				VK_NUMPAD1, VK_NUMPAD7, // VK_END VK_HOME
				VK_NUMPAD4, VK_NUMPAD8, // VK_LEFT VK_UP
				VK_NUMPAD6, VK_NUMPAD2, // VK_RIGHT VK_DOWN
			};
			if (!ext)
			{	if (vk >= VK_PRIOR && vk <= VK_DOWN)
					vk = vkN[vk - VK_PRIOR];
				else if (vk == VK_INSERT)  vk = VK_NUMPAD0;
				else if (vk == VK_DELETE)  vk = VK_DECIMAL;
			}else    if (vk == VK_RETURN)  vk = 0xFF;  // own

			//  get key id from vk
			int id = kk->vk2key[vk] - 1;
			if (id >= 0)
			{
				if (id >= vk_EXTRA)  id -= vk_EXTRA;
				Key& k = kk->keys[id];

				if (press)    k.on = true;
				if (release)  k.on = false;
			}
		}
	}
	return CallNextHookEx(hHook, nCode, wp, lp);
}


//  attach
void Keys::Hook()
{
	if (hHook != NULL)
		return;

	kk = this;

	//  hook kbd  * * *
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyHandler, NULL, 0);
}

void Keys::UnHook()
{
	kk = nullptr;

	//  unhook
	if (hHook != NULL)
		UnhookWindowsHookEx(hHook);
}

#endif
