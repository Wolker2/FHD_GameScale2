#!/usr/bin/env python3
"""Export FHD_GameScale2 v0.5.7 source without modifying the original ZIP."""
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path
from zipfile import ZipFile

ROOT=Path(__file__).resolve().parents[1]
ZIP=ROOT/"install/FHD_GameScale2_0.5.7_TEST.zip"
PREFIX="Mods/FHD_MODS/FHD_GameScale2/"
EXPECTED="916f251a96e2facd54ee6188c472c5daa7956452b631eaea9970dfc6744755de"
sys.path.insert(0,str(ROOT/"tools"))
import dat_codec

def hashdata(data): return hashlib.sha256(data).hexdigest()
def copy(rel,data):
    path=ROOT/rel
    assert path.resolve().is_relative_to(ROOT.resolve()),rel
    path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists(): assert path.read_bytes()==data, "Existing source changed: "+rel
    else: path.write_bytes(data)
    return hashdata(data)

def main():
    assert hashdata(ZIP.read_bytes())==EXPECTED,"Installer ZIP SHA256 mismatch"
    original={}; exported={}
    with ZipFile(ZIP) as z, tempfile.TemporaryDirectory() as tmp:
        assert z.testzip() is None
        entries=[item for item in z.infolist() if not item.is_dir()]
        assert len(z.infolist())==43 and len(entries)==39
        assert len(set(e.filename for e in entries))==39
        source_count=0
        gi=[]
        for item in entries:
            name=item.filename
            assert not name.startswith("/") and ".." not in Path(name).parts
            data=z.read(name)
            original[name]={"size":len(data),"sha256":hashdata(data)}
            if name.startswith(PREFIX+"SOURCE/"):
                target="SOURCE/"+name[len(PREFIX+"SOURCE/"):]
                exported[target]=copy(target,data)
                source_count+=1
            elif name==PREFIX+"CFG/CacheData.dat":
                project=dat_codec.decode(data,"cachedata")
                assert dat_codec.encode(project)==data
                target="CFG/CacheData.dat.json"
                exported[target]=copy(target,(json.dumps(project,ensure_ascii=False,indent=2)+"\n").encode())
            elif name==PREFIX+"ModuleInfo.txt":
                txt=data.decode("cp1251")
                assert txt.encode("cp1251")==data
                exported["docs/ModuleInfo.original.cp1251.txt"]=copy("docs/ModuleInfo.original.cp1251.txt",data)
                exported["docs/ModuleInfo.utf8.txt"]=copy("docs/ModuleInfo.utf8.txt",txt.encode("utf-8"))
            elif name==PREFIX+"README_RU.txt":
                exported["docs/README_RU.txt"]=copy("docs/README_RU.txt",data)
            elif name=="XenoNative/Plugins/XenoStable/README_RU.txt":
                exported["docs/XenoStable_README_RU.txt"]=copy("docs/XenoStable_README_RU.txt",data)
            elif name.startswith(PREFIX+"DATA/Radar/"):
                relative=name[len(PREFIX+"DATA/Radar/"):]
                assert relative.split("/")[0] in ("150","200","250")
                target="RADAR/ORIGINAL/"+relative
                exported[target]=copy(target,data)
                if relative.endswith(".gi"): gi.append((relative,data))
        assert source_count==12
        assert len(gi)==3
        assert sum(n.endswith(".gai") for n in original)==3
        for rel,raw in gi:
            scale=rel.split("/")[0]
            inp=Path(tmp)/scale
            inp.mkdir()
            gifile=inp/"Background.gi"
            gifile.write_bytes(raw)
            dest=ROOT/"RADAR/PNG"/scale
            dest.mkdir(parents=True,exist_ok=True)
            subprocess.run([sys.executable,"-m","srhd_modkit","convert","gi-png",str(gifile),"--output",str(dest)],check=True,capture_output=True,text=True)
            png=dest/"Background.png"
            assert png.is_file() and png.read_bytes().startswith(b"\x89PNG\r\n\x1a\n")
            exported["RADAR/PNG/"+scale+"/Background.png"]=hashdata(png.read_bytes())
    report={"version":"0.5.7_TEST","original_zip_sha256":EXPECTED,"original_files":39,
            "original_source_files":12,"radar_original_resource_files":12,
            "radar_background_PNG_exports":3,"cachedata_byte_exact_roundtrip":True,
            "original_files_sha256":original,"dll_compiled":False,"game_tested":False}
    (ROOT/"ORIGINAL_FILE_SHA256.json").write_text(json.dumps(original,ensure_ascii=False,indent=2)+"\n")
    (ROOT/"SOURCE_HASHES.json").write_text(json.dumps(exported,ensure_ascii=False,indent=2)+"\n")
    (ROOT/"VALIDATION_REPORT.json").write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n")
    print("PASS original files:",len(original),"sources:",source_count,"PNG:",len(gi))
if __name__=="__main__":main()
