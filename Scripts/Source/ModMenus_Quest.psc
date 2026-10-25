Scriptname ModMenus_Quest extends Quest
{Mod Menus: one controller-friendly list of every mod-added menu. Opens from Tween Menu Overhaul's MOD MENUS entry,
the mod event ModMenus_Open, or the key in ModMenus.ini. The list is UIExtensions' list menu (categories > menus);
the picked menu opens the way its mod listens: its own key pressed (SKSE Input), or its mod event sent. Menus without
gamepad support then get Pad Pointer (if installed), with their own keys as its close keys.}

bool busy = false
int openKey = 0

Event OnInit()
	Register()
EndEvent

; also called by ModMenus_PlayerAlias on every game load (registrations are not kept in the save)
Function Register()
	RegisterForModEvent("ModMenus_Open", "OnOpenRequest")
	If openKey > 0
		UnregisterForKey(openKey)
	EndIf
	openKey = ModMenusNative.GetOpenKey()
	If openKey > 0
		RegisterForKey(openKey)
	EndIf
	busy = false
EndFunction

Event OnOpenRequest(string eventName, string strArg, float numArg, Form sender)
	Open()
EndEvent

Event OnKeyDown(int keyCode)
	If keyCode == openKey && !Utility.IsInMenuMode()
		Open()
	EndIf
EndEvent

Function Open()
	If busy
		return
	EndIf
	busy = true
	string[] categories = ModMenusNative.GetCategories()
	string[] labels = ModMenusNative.GetLabels()
	int[] categoryOf = ModMenusNative.GetCategoryOf()
	If labels.Length == 0
		Debug.Notification("Mod Menus: no supported menu mods found")
		busy = false
		return
	EndIf
	int n = 0
	While UI.IsMenuOpen("TweenMenu") && n < 40   ; opened from the Tween menu: let it close first
		Utility.WaitMenuMode(0.05)
		n += 1
	EndWhile
	UIListMenu menu = UIExtensions.GetMenu("UIListMenu") as UIListMenu
	If !menu
		Debug.Notification("Mod Menus: UIExtensions is missing")
		busy = false
		return
	EndIf
	int c = 0
	While c < categories.Length
		int catEntry = menu.AddEntryItem(categories[c], -1, -1, true)
		int i = 0
		While i < labels.Length
			If categoryOf[i] == c
				menu.AddEntryItem(labels[i], catEntry, i, false)
			EndIf
			i += 1
		EndWhile
		c += 1
	EndWhile
	menu.OpenMenu()
	int pick = menu.GetResultFloat() as int   ; the entry's callback = its index (-1 = backed out)
	busy = false
	If pick >= 0 && pick < labels.Length
		Launch(pick, labels[pick])
	EndIf
EndFunction

Function Launch(int i, string label)
	int n = 0
	While UI.IsMenuOpen("CustomMenu") && n < 40
		Utility.WaitMenuMode(0.05)
		n += 1
	EndWhile
	int[] r = ModMenusNative.Resolve(i)
	If r[0] < 0
		Debug.Notification("Mod Menus: " + label + ": " + ModMenusNative.GetError(i))
		return
	EndIf
	string closeKeys = ""
	If r[0] == 1
		SendModEvent(ModMenusNative.GetEvent(i))
	Else
		If r[2] > 0
			Input.HoldKey(r[2])
			closeKeys = r[2] + "+"
		EndIf
		If r[3] > 0
			Input.HoldKey(r[3])
			closeKeys += r[3] + "+"
		EndIf
		Utility.WaitMenuMode(0.05)
		Input.TapKey(r[1])
		Utility.WaitMenuMode(0.05)
		If r[3] > 0
			Input.ReleaseKey(r[3])
		EndIf
		If r[2] > 0
			Input.ReleaseKey(r[2])
		EndIf
		closeKeys += r[1]
	EndIf
	If r[4] == 1
		SendModEvent("PadPointer_On", closeKeys)   ; Pad Pointer: gamepad as mouse for menus without pad support
	EndIf
EndFunction
