# Aeternum modules folder (`modules/`)

This folder is called `modules` (plural), not `module`.
The worldserver looks for it next to `worldserver.conf`.
If `Aeternum.Modules.ManifestDir` is empty, this directory is the one used.

## How to load a native module

1. Create a subfolder `modules/<id>/`, where `<id>` is the module identifier (for example `mod_anticheat`).
2. Put both files in that folder:
   - `module.manifest.json` (required)
   - the library named in the manifest (for example `mod_anticheat.dll`)
3. The Native Runtime is OFF by default (`Aeternum.Modules.NativeRuntime = 0`).
   Until you turn it on, drop-in files in `modules/` are NOT executed as native code.
4. To enable it, activate the configuration overlay:
   - copy `worldserver.conf.d/aeternum_features.conf.dist` to `worldserver.conf.d/aeternum_features.conf`
   - set `Aeternum.Modules.NativeRuntime = 1` in the `.conf` file
   - `.dist` files are NOT loaded: setting the flag only in the template has no effect
   - restart the worldserver

Required layout:

    modules/
      <id>/
        module.manifest.json
        <library>.dll

## Notes

- Do not rename this folder to `module`: the loader would not find it.
- `Aeternum.Modules.NativeRuntime` stays 0 until you explicitly enable it.
- This build includes `admin.monitoring` (a static module compiled into the worldserver, with a manifest).
- Native samples live in the source tree (`modules/native/`) and are not copied here automatically.
