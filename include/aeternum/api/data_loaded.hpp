#ifndef AETERNUM_API_DATA_LOADED_HPP
#define AETERNUM_API_DATA_LOADED_HPP
namespace Aeternum {
// Core-owned bootstrap seam; never exposed to native modules. Installed with ScriptMgr before loading finishes.
inline void (*DataLoadedBootHook)() = nullptr;
inline void OnDataLoaded() { if (DataLoadedBootHook) DataLoadedBootHook(); }
}
#endif
