Scriptname ModMenus_MCM extends MCM_ConfigBase
{Mod Menus settings page, served by MCM Helper (MCM/Config/ModMenus/config.json). The Tween entry and the open key are
ModSettings that ModMenus.dll reads; the Menus and Add a menu pages fill their lists from ModMenusNative and write
through it (SKSE/Plugins/ModMenus/User.json).}

string[] categories

Event OnConfigOpen()
	Fill()
EndEvent

; the lists that depend on what is installed: menus to pick, categories to add to
Function Fill()
	string[] labels = ModMenusNative.GetLabels()
	If labels.Length == 0
		labels = new string[1]
		labels[0] = "(none listed)"
	EndIf
	SetMenuOptions("iPick:Menus", labels)
	If GetModSettingInt("iPick:Menus") >= labels.Length
		SetModSettingInt("iPick:Menus", 0)
	EndIf
	ShowPick()
	string[] found = ModMenusNative.GetCategories()
	int extra = 0
	If found.Find("My Menus") < 0
		extra = 1
	EndIf
	categories = Utility.CreateStringArray(found.Length + extra)
	int i = 0
	While i < found.Length
		categories[i] = found[i]
		i += 1
	EndWhile
	If extra == 1
		categories[found.Length] = "My Menus"
	EndIf
	SetMenuOptions("iCategory:Add", categories)
	If GetModSettingInt("iCategory:Add") >= categories.Length
		SetModSettingInt("iCategory:Add", 0)
	EndIf
EndFunction

Function ShowPick()
	SetModSettingString("sKey:Menus", ModMenusNative.Describe(GetModSettingInt("iPick:Menus")))
EndFunction

Event OnSettingChange(string a_ID)
	If a_ID == "iTweenSection:General" || a_ID == "iTweenPosition:General"
		ModMenusNative.SetTween()
		ShowMessage("Saved. The Tween menu shows MOD MENUS there from the next time the game starts.", false)
	ElseIf a_ID == "iOpenKey:General"
		ModMenus_Quest main = Quest.GetQuest("ModMenusQuest") as ModMenus_Quest
		If main
			main.Register()
		EndIf
	ElseIf a_ID == "iPick:Menus"
		ShowPick()
		RefreshMenu()
	EndIf
EndEvent

Function Done(string error, string success)
	If error != ""
		ShowMessage("Mod Menus: " + error, false)
		return
	EndIf
	Fill()
	RefreshMenu()
	ShowMessage(success, false)
EndFunction

Function ReloadEntries()
	ModMenusNative.Reload()
	Done("", ModMenusNative.GetLabels().Length + " menus listed.")
EndFunction

Function UseKey()
	int i = GetModSettingInt("iPick:Menus")
	Done(ModMenusNative.SetKey(i, GetModSettingInt("iKey:Menus"), GetModSettingBool("bShift:Menus"), \
		GetModSettingBool("bCtrl:Menus"), GetModSettingBool("bAlt:Menus")), "Saved: " + ModMenusNative.Describe(i))
EndFunction

Function ClearKey()
	int i = GetModSettingInt("iPick:Menus")
	Done(ModMenusNative.ClearKey(i), "Back to the mod's own key: " + ModMenusNative.Describe(i))
EndFunction

Function RemoveMenu()
	string label = ModMenusNative.GetLabels()[GetModSettingInt("iPick:Menus")]
	If ShowMessage("Remove " + label + " from the list?", true, "Remove", "Cancel")
		Done(ModMenusNative.RemoveMenu(GetModSettingInt("iPick:Menus")), label + " removed.")
	EndIf
EndFunction

Function AddMenu()
	string label = GetModSettingString("sLabel:Add")
	string category = categories[GetModSettingInt("iCategory:Add")]
	string error = ModMenusNative.AddMenu(label, category, GetModSettingInt("iKey:Add"), GetModSettingBool("bShift:Add"), \
		GetModSettingBool("bCtrl:Add"), GetModSettingBool("bAlt:Add"), GetModSettingString("sEvent:Add"), \
		GetModSettingBool("bPointer:Add"))
	If error == ""
		SetModSettingString("sLabel:Add", "")
		SetModSettingString("sEvent:Add", "")
		SetModSettingInt("iKey:Add", 0)
	EndIf
	Done(error, label + " added under " + category + ".")
EndFunction
