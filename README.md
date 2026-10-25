# Mod Menus

One controller-friendly list that opens every mod-added menu. No more hunting for each mod's hotkey (or reaching for
the keyboard): open **MOD MENUS** from the Tween menu, pick a category, pick a menu.

- **The menus come from small entry files,** one per supported mod. A menu is only listed when its mod is installed.
- **Keys are read from each mod's own config file every time you pick its menu.** Rebind a menu in its own settings
  and Mod Menus follows; there's nothing to set up here.
- **Menus that don't support a controller** turn on [Pad Pointer](https://github.com/MyDataTrends/PadPointer) if you have
  it, so you can steer them with the left stick. Its Start button presses the menu's own close key.
- Opens from Tween Menu Overhaul's **MOD MENUS** entry, the Pause key (configurable), or the mod event
  `ModMenus_Open` (bind it to anything that sends mod events).

## Supported out of the box
| Category | Menu | Mod |
|---|---|---|
| Graphics | Community Shaders | [Community Shaders](https://www.nexusmods.com/skyrimspecialedition/mods/86492) |
| Graphics | Photo Mode | [Photo Mode](https://www.nexusmods.com/skyrimspecialedition/mods/91701) |
| Characters & Gear | Equipment Displays | [Immersive Equipment Displays](https://www.nexusmods.com/skyrimspecialedition/mods/62001) |
| Characters & Gear | Party Sheet | [Skyrim Party Sheet](https://www.nexusmods.com/skyrimspecialedition/mods/167538) |
| Characters & Gear | Character Sheet | [Skyrim Character Sheet](https://www.nexusmods.com/skyrimspecialedition/mods/56069) |
| Characters & Gear | NPC Presets | [NPA - NPC Preset Applier](https://www.nexusmods.com/skyrimspecialedition/mods/193575) |
| Characters & Gear | Body Presets | [OBody Next Generation](https://www.nexusmods.com/skyrimspecialedition/mods/77016) |
| Characters & Gear | Achievements | [Achievement Injector](https://www.nexusmods.com/skyrimspecialedition/mods/126220) |
| Animation | Open Animation Replacer | [Open Animation Replacer](https://www.nexusmods.com/skyrimspecialedition/mods/92109) |
| Mod Settings | SKSE Menu Framework | [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) |
| Mod Settings | dMenu | [dMenu NG](https://www.nexusmods.com/skyrimspecialedition/mods/166751) |
| Mod Settings | Equipment Cycle Hotkeys | [Equipment Cycle Hotkeys](https://www.nexusmods.com/skyrimspecialedition/mods/108602) |

Note: OBody and Achievement Injector use fixed keys (Shift+O, Shift+N) that other mods may also use out of the box
(Open Animation Replacer's default is Shift+O). Rebind the others in their own settings if two menus open at once.

## Adding a menu
Drop a JSON file into `SKSE/Plugins/ModMenus/` (your mod can ship one). Example:
```json
{
  "menus": [
    {
      "id": "mymenu",
      "label": "My Menu",
      "category": "Mod Settings",
      "requires": ["SKSE/Plugins/MyMod.dll"],
      "pointer": true,
      "open": { "key": { "file": "SKSE/Plugins/MyMod.ini", "ini": "Hotkeys.OpenMenu", "format": "dik", "default": "F9" } }
    }
  ]
}
```
- `requires`: files under Data (or plugin names) that must be present for the entry to show.
- `open` is one of:
  - `{ "modevent": "MyMod_Open" }`: sends that mod event;
  - `{ "key": { "fixed": "Shift+O" } }`: a key the mod hardcodes;
  - `{ "key": { "file": ..., "ini": "Section.Key" | "json": "path.to.value", "format": ..., "default": ... } }`: the
    key read from the mod's config. `file` may be a list of candidates (the first that exists is read); `default`
    applies while the mod hasn't written its config yet; JSON may contain `//` comments and trailing commas; numbers
    in a JSON path index arrays (`keysets.0`).
- Formats: `dik` (a DirectInput scan code; optional `"shift"`/`"ctrl"`/`"alt"` name ini flags that add modifiers),
  `dik_list` (a list, or `"42,24"`), `vk` (Windows virtual-key codes), `key_name` (`"F1"`), `combo_names`
  (`"Shift+N"`), `name_list` (`["LShift", "\\"]`).
- `pointer`: turn on Pad Pointer after opening (for menus without controller support).
- What was found, skipped and read is written to `Documents/My Games/Skyrim Special Edition/SKSE/ModMenus.log`.

## Settings
`SKSE/Plugins/ModMenus.ini`:
- `[General] uOpenKey`: the keyboard key that opens the list (DirectInput scan code; 197 = Pause, 0 = none).
- `[Keys]`: per entry id, a key to use instead of reading it (only for a menu whose key can't be read), e.g.
  `obody = Shift+O`.

## Requirements
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [UIExtensions](https://www.nexusmods.com/skyrimspecialedition/mods/17561) (the list menu)
- Recommended: [Tween Menu Overhaul](https://www.nexusmods.com/skyrimspecialedition/mods/143275) (the MOD MENUS entry),
  [Pad Pointer](https://github.com/MyDataTrends/PadPointer) (controller as a mouse in menus that need one)

Built with CommonLibSSE-NG for Skyrim SE and AE. Tested on Skyrim 1.6.1170 (Steam).

## Building
`plugin/`: Visual Studio 2022 with vcpkg, `cmake --preset relwithdebinfo` and
`cmake --build build/relwithdebinfo --config RelWithDebInfo`. `Scripts/Source/`: Papyrus (imports UIExtensions'
sources). `ModMenus.esp` (one quest) is built by the author's own tooling and included in the release; `data/` holds
the entry files, the ini and the Tween Menu Overhaul entry.

## License
MIT, see [LICENSE](LICENSE). Use it, change it, include it in your lists and mods.
