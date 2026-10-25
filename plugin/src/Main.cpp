// Mod Menus: one controller-friendly list of every mod-added menu (user, 2026-10-19: "one controller-friendly place
// that opens every mod-added menu"; public version 2026-10-24).
//
// Entries come from Data/SKSE/Plugins/ModMenus/*.json (one file per supported mod; anyone can add one). An entry is
// listed only when the files or plugins it requires are present. When the list opens, each picked menu's key is read
// from that mod's own config file, so a rebound key follows without touching this mod. The Papyrus side
// (ModMenus_Quest) shows the list (UIExtensions) and opens the picked menu: it presses the keys returned by Resolve
// (SKSE Input.HoldKey/TapKey) or sends the menu's mod event, then turns on Pad Pointer for menus without gamepad
// support (mod event PadPointer_On with the menu's own keys as its close keys).
//
// Entry (all strings; "open" is one of key / modevent):
//   { "id", "label", "category", "requires": ["SKSE/Plugins/X.dll", "X.esp"], "pointer": true,
//     "open": { "modevent": "Some_Open" } |
//             { "key": { "fixed": "Shift+O" } } |
//             { "key": { "file": "SKSE/Plugins/X.ini", "ini": "Section.Key", "format": "dik",
//                        "shift": "Section.ShiftKey", "ctrl": ..., "alt": ... } } |
//             { "key": { "file": "SKSE/Plugins/X.json", "json": "a.b.0", "format": "vk" } } }
//   "file" may list candidates (the first that exists is read); "default": "Shift+O" applies while the mod has not
//   written its config yet (several do that on first run).
// Formats: dik (number; + optional shift/ctrl/alt ini flags), dik_list (list or "a,b"), vk (number or list of Windows
// virtual-key codes), key_name ("F1"), combo_names ("Shift+N"), name_list (list of names). JSON files may hold //
// comments and trailing commas. Paths with numbers index into arrays ("menu_toggle_keysets.0").
#include "PCH.h"

#include <Windows.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	namespace fs = std::filesystem;
	using json = nlohmann::json;

	const fs::path DATA = "Data";
	const fs::path ENTRIES = DATA / "SKSE" / "Plugins" / "ModMenus";
	const char* INI = ".\\Data\\SKSE\\Plugins\\ModMenus.ini";

	struct Entry
	{
		std::string id, label, category, file;
		json open;
		bool pointer = false;
	};

	std::vector<Entry> g_entries;
	std::vector<std::string> g_categories;

	struct Resolved
	{
		int kind = -1;            // 0 keys, 1 mod event, -1 error
		std::vector<int> keys;    // DirectInput scan codes, modifiers first
		std::string event, error;
	};

	std::string Lower(std::string s)
	{
		std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	}

	std::string Trim(const std::string& s)
	{
		auto a = s.find_first_not_of(" \t\r\n");
		auto b = s.find_last_not_of(" \t\r\n");
		return a == std::string::npos ? "" : s.substr(a, b - a + 1);
	}

	std::vector<std::string> Split(const std::string& s, const std::string& seps)
	{
		std::vector<std::string> out;
		std::string cur;
		for (char c : s) {
			if (seps.find(c) != std::string::npos) {
				if (!Trim(cur).empty()) out.push_back(Trim(cur));
				cur.clear();
			} else {
				cur += c;
			}
		}
		if (!Trim(cur).empty()) out.push_back(Trim(cur));
		return out;
	}

	// ── keys ───────────────────────────────────────────────────────────────────────────────────────────────────
	const std::vector<int> MODIFIERS = { 29, 42, 54, 56, 157, 184 };
	bool IsModifier(int k) { return std::ranges::find(MODIFIERS, k) != MODIFIERS.end(); }

	const std::map<std::string, int> NAMES = {
		{ "esc", 1 }, { "escape", 1 }, { "backspace", 14 }, { "tab", 15 }, { "enter", 28 }, { "lctrl", 29 },
		{ "ctrl", 29 }, { "control", 29 }, { "lshift", 42 }, { "shift", 42 }, { "\\", 43 }, { "backslash", 43 },
		{ "rshift", 54 }, { "lalt", 56 }, { "alt", 56 }, { "space", 57 }, { "f1", 59 }, { "f2", 60 }, { "f3", 61 },
		{ "f4", 62 }, { "f5", 63 }, { "f6", 64 }, { "f7", 65 }, { "f8", 66 }, { "f9", 67 }, { "f10", 68 },
		{ "f11", 87 }, { "f12", 88 }, { "rctrl", 157 }, { "ralt", 184 }, { "home", 199 }, { "up", 200 },
		{ "pageup", 201 }, { "pgup", 201 }, { "left", 203 }, { "right", 205 }, { "end", 207 }, { "down", 208 },
		{ "pagedown", 209 }, { "pgdn", 209 }, { "insert", 210 }, { "ins", 210 }, { "delete", 211 }, { "del", 211 },
		{ "pause", 197 }, { "`", 41 }, { "grave", 41 }, { "tilde", 41 }
	};

	// Extended keys: MapVirtualKeyW(VK_END, MAPVK_VK_TO_VSC_EX) returned 0x4F without the E0 prefix (Numpad 1 instead
	// of End), so these are mapped explicitly.
	const std::map<int, int> VK_EXTENDED = { { 0x21, 201 }, { 0x22, 209 }, { 0x23, 207 }, { 0x24, 199 }, { 0x25, 203 },
		{ 0x26, 200 }, { 0x27, 205 }, { 0x28, 208 }, { 0x2C, 183 }, { 0x2D, 210 }, { 0x2E, 211 }, { 0x5B, 219 },
		{ 0x5C, 220 }, { 0x5D, 221 }, { 0x6F, 181 }, { 0x90, 69 }, { 0xA3, 157 }, { 0xA5, 184 } };

	int VkToDik(int vk)
	{
		if (auto it = VK_EXTENDED.find(vk); it != VK_EXTENDED.end()) return it->second;
		UINT sc = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC_EX);
		if (!sc) throw std::runtime_error(std::format("no scan code for virtual key {}", vk));
		return static_cast<int>((sc & 0x7F) | (((sc >> 8) == 0xE0 || (sc >> 8) == 0xE1) ? 0x80 : 0));
	}

	int NameToDik(const std::string& name)
	{
		auto n = Lower(Trim(name));
		if (auto it = NAMES.find(n); it != NAMES.end()) return it->second;
		if (n.size() == 1) {  // letters, digits, punctuation: through the keyboard layout
			SHORT vk = VkKeyScanW(static_cast<wchar_t>(n[0]));
			if (vk != -1) return VkToDik(vk & 0xFF);
		}
		throw std::runtime_error(std::format("unknown key name '{}'", name));
	}

	int Number(const json& v)
	{
		if (v.is_number()) return v.get<int>();
		return std::stoi(Trim(v.get<std::string>()), nullptr, 0);  // "0x0E" and "14" alike
	}

	// ── config files ────────────────────────────────────────────────────────────────────────────────────────────
	std::string ReadText(const fs::path& p)
	{
		std::ifstream f(p, std::ios::binary);
		if (!f) throw std::runtime_error(std::format("{} not found", p.string()));
		std::stringstream ss;
		ss << f.rdbuf();
		auto s = ss.str();
		if (s.rfind("\xEF\xBB\xBF", 0) == 0) s.erase(0, 3);
		return s;
	}

	std::string IniGet(const fs::path& p, const std::string& dotted)
	{
		auto dot = dotted.find('.');
		auto sec = Lower(dotted.substr(0, dot)), key = Lower(dotted.substr(dot + 1));
		std::istringstream in(ReadText(p));
		std::string line, cur;
		while (std::getline(in, line)) {
			auto s = Trim(line);
			if (s.size() > 1 && s.front() == '[' && s.back() == ']') {
				cur = Lower(s.substr(1, s.size() - 2));
			} else if (cur == sec && !s.empty() && s[0] != ';' && s[0] != '#') {
				auto eq = s.find('=');
				if (eq != std::string::npos && Lower(Trim(s.substr(0, eq))) == key) {
					auto v = s.substr(eq + 1);
					return Trim(v.substr(0, v.find(';')));
				}
			}
		}
		throw std::runtime_error(std::format("{} not in {}", dotted, p.filename().string()));
	}

	// JSON with // comments and trailing commas (Equipment Cycle Hotkeys writes both)
	json ParseLoose(const std::string& text)
	{
		std::string out;
		bool inStr = false;
		for (size_t i = 0; i < text.size(); ++i) {
			char c = text[i];
			if (inStr) {
				out += c;
				if (c == '\\' && i + 1 < text.size()) out += text[++i];
				else if (c == '"') inStr = false;
			} else if (c == '"') {
				inStr = true;
				out += c;
			} else if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
				while (i < text.size() && text[i] != '\n') ++i;
				out += '\n';
			} else {
				out += c;
			}
		}
		std::string clean;  // drop commas that only whitespace separates from a closing bracket
		for (size_t i = 0; i < out.size(); ++i) {
			if (out[i] == ',') {
				size_t j = out.find_first_not_of(" \t\r\n", i + 1);
				if (j != std::string::npos && (out[j] == '}' || out[j] == ']')) continue;
			}
			clean += out[i];
		}
		return json::parse(clean);
	}

	json JsonGet(json v, const std::string& dotted)
	{
		for (auto& part : Split(dotted, ".")) {
			if (v.is_array()) v = v.at(std::stoul(part));
			else v = v.at(part);
		}
		return v;
	}

	// ── resolving an entry ──────────────────────────────────────────────────────────────────────────────────────
	Resolved Resolve(const Entry& e)
	{
		Resolved r;
		try {
			if (e.open.contains("modevent")) {
				r.kind = 1;
				r.event = e.open["modevent"].get<std::string>();
				return r;
			}
			const json& k = e.open.at("key");
			std::vector<int> keys;
			// ModMenus.ini [Keys] <id> = Shift+O: the user's override for a menu whose key can't be read (or reads wrong)
			char over[64]{};
			GetPrivateProfileStringA("Keys", e.id.c_str(), "", over, sizeof(over), INI);
			if (over[0]) {
				for (auto& t : Split(over, "+")) keys.push_back(NameToDik(t));
				std::ranges::stable_partition(keys, IsModifier);
				r.kind = 0;
				r.keys = keys;
				return r;
			}
			// "file": one path or candidates (the first that exists); "default": the combo when the mod has not written
			// its config yet (several write it on first run)
			fs::path p;
			std::error_code ec;
			if (k.contains("file")) {
				for (auto& f : k["file"].is_array() ? k["file"] : json::array({ k["file"] })) {
					if (fs::exists(DATA / f.get<std::string>(), ec)) {
						p = DATA / f.get<std::string>();
						break;
					}
				}
			}
			json raw;
			bool haveRaw = false;
			if (!p.empty()) {
				try {
					raw = k.contains("ini") ? json(IniGet(p, k["ini"].get<std::string>()))
					                        : JsonGet(ParseLoose(ReadText(p)), k.at("json").get<std::string>());
					haveRaw = true;
				} catch (const std::exception&) {
					if (!k.contains("default")) throw;
				}
			}
			std::string fixed = k.value("fixed", "");
			if (fixed.empty() && !haveRaw) {
				if (!k.contains("default")) throw std::runtime_error("its config file was not found");
				fixed = k["default"].get<std::string>();
			}
			if (!fixed.empty()) {  // the mod has no key setting (or none written yet): the key it listens to
				for (auto& t : Split(fixed, "+")) keys.push_back(NameToDik(t));
			} else {
				auto fmt = k.at("format").get<std::string>();
				auto list = [&](const json& v) {
					std::vector<json> out;
					if (v.is_array()) for (auto& x : v) out.push_back(x);
					else if (v.is_string()) for (auto& t : Split(v.get<std::string>(), ",+ ")) out.push_back(t);
					else out.push_back(v);
					return out;
				};
				if (fmt == "dik") {
					keys.push_back(Number(raw));
					for (auto [flag, dik] : { std::pair{ "ctrl", 29 }, { "shift", 42 }, { "alt", 56 } }) {
						if (k.contains(flag) && std::stoi("0" + IniGet(p, k[flag].get<std::string>())) != 0) keys.insert(keys.begin(), dik);
					}
				} else if (fmt == "dik_list") {
					for (auto& v : list(raw)) keys.push_back(Number(v));
				} else if (fmt == "vk") {
					for (auto& v : list(raw)) keys.push_back(VkToDik(Number(v)));
				} else if (fmt == "key_name") {
					keys.push_back(NameToDik(raw.get<std::string>()));
				} else if (fmt == "combo_names") {
					for (auto& t : Split(raw.get<std::string>(), "+")) keys.push_back(NameToDik(t));
				} else if (fmt == "name_list") {
					for (auto& v : raw) keys.push_back(NameToDik(v.get<std::string>()));
				} else {
					throw std::runtime_error("unknown format " + fmt);
				}
			}
			std::ranges::stable_partition(keys, IsModifier);
			auto mains = std::ranges::count_if(keys, [](int x) { return !IsModifier(x); });
			if (keys.empty() || mains != 1 || keys.size() > 3) throw std::runtime_error("needs one key and up to two modifiers");
			r.kind = 0;
			r.keys = keys;
		} catch (const std::exception& ex) {
			r.kind = -1;
			r.error = ex.what();
		}
		return r;
	}

	// ── loading the entries ─────────────────────────────────────────────────────────────────────────────────────
	bool Present(const std::string& req)
	{
		auto ext = Lower(fs::path(req).extension().string());
		if (ext == ".esp" || ext == ".esm" || ext == ".esl") {
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh && dh->LookupModByName(req) != nullptr;
		}
		std::error_code ec;
		return fs::exists(DATA / req, ec);
	}

	void Load()
	{
		g_entries.clear();
		g_categories.clear();
		std::vector<fs::path> files;
		std::error_code ec;
		for (auto& f : fs::directory_iterator(ENTRIES, ec)) {
			if (Lower(f.path().extension().string()) == ".json") files.push_back(f.path());
		}
		std::ranges::sort(files);
		for (auto& f : files) {
			try {
				json doc = ParseLoose(ReadText(f));
				for (auto& m : doc.value("menus", json::array())) {
					Entry e{ m.value("id", ""), m.value("label", ""), m.value("category", "Other"), f.filename().string(),
						m.value("open", json::object()), m.value("pointer", false) };
					bool ok = !e.label.empty() && (e.open.contains("key") || e.open.contains("modevent"));
					std::string missing;
					for (auto& req : m.value("requires", json::array())) {
						if (!Present(req.get<std::string>())) missing = req.get<std::string>();
					}
					if (!ok) {
						LOG_WARN("{}: entry '{}' has no label or no way to open it; skipped", e.file, e.id);
					} else if (!missing.empty()) {
						LOG_INFO("{}: '{}' not listed ({} not found)", e.file, e.label, missing);
					} else {
						if (std::ranges::find(g_categories, e.category) == g_categories.end()) g_categories.push_back(e.category);
						LOG_INFO("{}: '{}' listed under {}", e.file, e.label, e.category);
						g_entries.push_back(std::move(e));
					}
				}
			} catch (const std::exception& ex) {
				LOG_WARN("{}: not read ({})", f.filename().string(), ex.what());
			}
		}
		LOG_INFO("{} menus in {} categories from {} files", g_entries.size(), g_categories.size(), files.size());
	}

	// ── Papyrus: ModMenusNative ─────────────────────────────────────────────────────────────────────────────────
	std::vector<std::string> GetCategories(RE::StaticFunctionTag*) { return g_categories; }

	std::vector<std::string> GetLabels(RE::StaticFunctionTag*)
	{
		std::vector<std::string> out;
		for (auto& e : g_entries) out.push_back(e.label);
		return out;
	}

	std::vector<std::int32_t> GetCategoryOf(RE::StaticFunctionTag*)
	{
		std::vector<std::int32_t> out;
		for (auto& e : g_entries) {
			out.push_back(static_cast<std::int32_t>(std::ranges::find(g_categories, e.category) - g_categories.begin()));
		}
		return out;
	}

	// [kind (0 keys, 1 mod event, -1 error), main key, modifier 1, modifier 2, pointer (0/1)]; keys are read now
	std::vector<std::int32_t> ResolveMenu(RE::StaticFunctionTag*, std::int32_t i)
	{
		if (i < 0 || i >= static_cast<std::int32_t>(g_entries.size())) return { -1, 0, 0, 0, 0 };
		auto& e = g_entries[i];
		auto r = Resolve(e);
		if (r.kind < 0) {
			LOG_WARN("'{}': key not read: {}", e.label, r.error);
			return { -1, 0, 0, 0, 0 };
		}
		std::vector<int> mods;
		int main = 0;
		for (int k : r.keys) (IsModifier(k) ? (void)mods.push_back(k) : (void)(main = k));
		LOG_INFO("'{}': {}", e.label, r.kind == 1 ? "mod event " + r.event : std::format("keys main {} mods {}", main, mods.size()));
		return { r.kind, main, mods.size() > 0 ? mods[0] : 0, mods.size() > 1 ? mods[1] : 0, e.pointer ? 1 : 0 };
	}

	std::string GetEvent(RE::StaticFunctionTag*, std::int32_t i)
	{
		if (i < 0 || i >= static_cast<std::int32_t>(g_entries.size())) return "";
		auto r = Resolve(g_entries[i]);
		return r.kind == 1 ? r.event : "";
	}

	std::string GetError(RE::StaticFunctionTag*, std::int32_t i)
	{
		if (i < 0 || i >= static_cast<std::int32_t>(g_entries.size())) return "no such menu";
		return Resolve(g_entries[i]).error;
	}

	std::int32_t GetOpenKey(RE::StaticFunctionTag*)
	{
		return static_cast<std::int32_t>(GetPrivateProfileIntA("General", "uOpenKey", 197, INI));  // Pause
	}

	void Reload(RE::StaticFunctionTag*) { Load(); }

	bool RegisterFunctions(RE::BSScript::IVirtualMachine* vm)
	{
		const char* cls = "ModMenusNative";
		vm->RegisterFunction("GetCategories", cls, GetCategories);
		vm->RegisterFunction("GetLabels", cls, GetLabels);
		vm->RegisterFunction("GetCategoryOf", cls, GetCategoryOf);
		vm->RegisterFunction("Resolve", cls, ResolveMenu);
		vm->RegisterFunction("GetEvent", cls, GetEvent);
		vm->RegisterFunction("GetError", cls, GetError);
		vm->RegisterFunction("GetOpenKey", cls, GetOpenKey);
		vm->RegisterFunction("Reload", cls, Reload);
		return true;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse);
	if (auto dir = SKSE::log::log_directory()) {
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*dir / "ModMenus.log").string(), true);
		auto logger = std::make_shared<spdlog::logger>("ModMenus", std::move(sink));
		logger->set_level(spdlog::level::info);
		logger->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(logger));
	}
	LOG_INFO("Mod Menus 1.0.0 loading");
	SKSE::GetPapyrusInterface()->Register(RegisterFunctions);
	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* msg) {
		if (msg->type == SKSE::MessagingInterface::kDataLoaded) {
			Load();
		}
	});
	return true;
}
