#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
python3 compile_srhd_dat.py cache CacheData.txt ../CFG/CacheData.dat
output="${1:-../Native/FHD_GameScale2.XenoPlugin.dll}"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

flags=(-m32 -O2 -std=c11 -ffreestanding -fno-builtin -fno-pic -fno-pie
       -fno-stack-protector -fno-asynchronous-unwind-tables -fno-ident
       -fshort-wchar -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2
       -mno-sse -mno-mmx -Wall -Wextra -Werror)
gcc "${flags[@]}" -c game_scale.c -o "$work/game_scale.elf"
python3 elf_to_coff.py "$work/game_scale.elf" "$work/game_scale.obj"

cat > "$work/import_stub.c" <<'STUB'
#define WINAPI __attribute__((stdcall))
int WINAPI VirtualProtect(void *a, unsigned b, unsigned c, unsigned *d)
{ return a != d && b != c; }
int WINAPI FlushInstructionCache(void *a, const void *b, unsigned c)
{ return a != b && c != 0; }
void *WINAPI GetCurrentProcess(void) { return 0; }
int WINAPI GetModuleHandleExW(unsigned a, const void *b, void **c)
{ return a && b && c; }
STUB
gcc "${flags[@]}" -c "$work/import_stub.c" -o "$work/import_stub.elf"
python3 elf_to_coff.py "$work/import_stub.elf" "$work/import_stub.obj"
ld -m i386pe --shared --entry 0 --no-leading-underscore --enable-stdcall-fixup --kill-at \
    --out-implib "$work/kernel32.a" -o "$work/kernel32_stub.dll" \
    kernel32.def "$work/import_stub.obj"
cat > "$work/user32_import_stub.c" <<'STUB'
#define WINAPI __attribute__((stdcall))
int WINAPI GetClipCursor(void *r) { return r != 0; }
int WINAPI ClipCursor(const void *r) { return r != 0; }
int WINAPI GetClientRect(void *window, void *bounds) { return window && bounds; }
int WINAPI ClientToScreen(void *window, void *origin) { return window && origin; }
void *WINAPI GetForegroundWindow(void) { return 0; }
STUB
gcc "${flags[@]}" -c "$work/user32_import_stub.c" -o "$work/user32_import_stub.elf"
python3 elf_to_coff.py "$work/user32_import_stub.elf" "$work/user32_import_stub.obj"
ld -m i386pe --shared --entry 0 --no-leading-underscore --enable-stdcall-fixup --kill-at \
    --out-implib "$work/user32.a" -o "$work/user32_stub.dll" \
    user32.def "$work/user32_import_stub.obj"
ld -m i386pe --shared --entry 0 --no-leading-underscore --enable-stdcall-fixup --no-insert-timestamp \
    --enable-reloc-section --dynamicbase --nxcompat --subsystem windows --strip-all \
    -o "$output" game_scale.def "$work/game_scale.obj" "$work/kernel32.a" "$work/user32.a"
