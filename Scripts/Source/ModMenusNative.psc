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

; MCM (ModMenus_MCM). Settings live in MCM/Settings/ModMenus.ini over MCM/Config/ModMenus/settings.ini; your keys and
; the menus added in the MCM in SKSE/Plugins/ModMenus/User.json. The string functions return "" when done, else why not.
string Function Describe(int i) global native     ; what picking menu i does now: "Shift+O (from X.ini)", "mod event X"
bool Function IsUserMenu(int i) global native     ; added in the MCM (can be removed there)
string Function SetKey(int i, int keyCode, bool shift, bool ctrl, bool alt) global native
string Function ClearKey(int i) global native     ; back to the key read from the mod's own config
string Function AddMenu(string label, string category, int keyCode, bool shift, bool ctrl, bool alt, string modEvent, bool pointer) global native
string Function RemoveMenu(int i) global native
Function SetTween() global native                 ; rewrite the Tween Menu Overhaul entry from the settings
