Scriptname ModMenus_PlayerAlias extends ReferenceAlias
{Mod Menus: re-registers the open event and key on every game load.}

Event OnPlayerLoadGame()
	(GetOwningQuest() as ModMenus_Quest).Register()
EndEvent
