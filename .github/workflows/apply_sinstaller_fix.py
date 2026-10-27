from pathlib import Path

path = Path("projects/sInstaller/source/main.cpp")
s = path.read_text()

old = "    romfsInit();\n"
new = "    bool romfsMounted = R_SUCCEEDED(romfsInit());\n"
if old not in s:
    raise SystemExit("startup romfsInit pattern not found")
s = s.replace(old, new, 1)

old = "        if (screen == Screen::Extracting) {\n"
new = '''        if (screen == Screen::Extracting) {
            // The release zip contains switch/sInstaller/sInstaller.nro. When
            // this installer is launched from that path, its embedded RomFS
            // keeps the running NRO open and prevents ExtractZip() from
            // replacing it. The update path no longer needs romfs:/payload or
            // the bundled UI assets, so release the mount before extracting.
            if (romfsMounted) {
                romfsExit();
                romfsMounted = false;
            }

'''
if old not in s:
    raise SystemExit("extracting screen pattern not found")
s = s.replace(old, new, 1)

old = '''            ok = ExtractZip("sdmc:/slaunch_update.zip");
            remove("sdmc:/slaunch_update.zip");
'''
new = '''            ok = ExtractZip("sdmc:/slaunch_update.zip");
            remove("sdmc:/slaunch_update.zip");

            // A failed update returns to the main menu, where Install/Re-install
            // still needs romfs:/payload. Restore the mount in that case.
            if (!ok && !romfsMounted)
                romfsMounted = R_SUCCEEDED(romfsInit());
'''
if old not in s:
    raise SystemExit("extract/remove pattern not found")
s = s.replace(old, new, 1)

old = "    romfsExit();\n    return 0;\n"
new = "    if (romfsMounted) romfsExit();\n    return 0;\n"
if old not in s:
    raise SystemExit("cleanup romfsExit pattern not found")
s = s.replace(old, new, 1)

path.write_text(s)
