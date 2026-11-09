"""Compile actual C to WASM with pinned external WASI SDK 24 (clang 18.1.2).
No sysroot/libc/WASI imports. Output and command/hash evidence are in build dir.
Usage: python build.py <wasi-sdk/bin/clang.exe> <output-dir>
"""
import pathlib,subprocess,sys,json,hashlib
clang=pathlib.Path(sys.argv[1]);out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
source=pathlib.Path(__file__).with_name('mod_test.c')
variants={'infinite_loop':['-DINFINITE_LOOP'], 'call_loop':['-DCALL_LOOP'], 'finite_loop':['-DFINITE_LOOP'], 'mod_test':[], 'bad_pointer':['-DBAD_POINTER'], 'null_pointer':['-DNULL_POINTER'],
 'bad_signature':['-DBAD_SIGNATURE'], 'bad_entry_signature':['-DBAD_ENTRY_SIGNATURE'], 'last_byte':['-DLAST_BYTE'], 'unterminated':['-DUNTERMINATED'], 'format_data':['-DFORMAT_DATA'],
 'memory_grow':['-DMEMORY_GROW'], 'missing_entry':['-DEXPORT_NAME="not_on_init"'],
 'forbidden_import':['-DIMPORT_MODULE="wasi_snapshot_preview1"'],
 'unknown_import':['-DIMPORT_NAME="filesystem_open"'], 'oversized_memory':[]}
records={}
for name,defines in variants.items():
 target=out/(name+'.wasm')
 pages=32 if name=='oversized_memory' else 16
 args=[str(clang),'--target=wasm32','-nostdlib','-O2','-fno-ident','-fno-builtin','-Wl,--no-entry',
 '-Wl,--strip-all','-Wl,--initial-memory='+str(131072 if pages==16 else 2097152),
 '-Wl,--max-memory='+str(pages*65536),'-Wl,-z,stack-size=16384',*defines,str(source),'-o',str(target)]
 subprocess.run(args,check=True)
 records[name]={'command':args,'sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'bytes':target.stat().st_size}
records['compiler']=subprocess.check_output([str(clang),'--version'],text=True)
records['source_sha256']=hashlib.sha256(source.read_bytes()).hexdigest()
(out/'build_provenance.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
print(json.dumps(records,indent=2))
