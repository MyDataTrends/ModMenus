Scriptname ModMenusNative Hidden
{Mod Menus: functions served by ModMenus.dll. Entries come from Data/SKSE/Plugins/ModMenus/*.json.}

string[] Function GetCategories() global native
string[] Function GetLabels() global native
int[] Function GetCategoryOf() global native     ; per menu: index into GetCategories()
; per menu, keys read now from its mod's config: [kind (0 keys, 1 mod event, -1 error), main, modifier 1, modifier 2, pointer]
int[] Function Resolve(int i) global native
string Function GetEvent(int i) global native
string Function GetError(int i) global native
int Function GetOpenKey() global native          ; ModMenus.ini [General] uOpenKey (0 = none)
Function Reload() global native                  ; read the entry files again
