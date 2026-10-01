"""Fetch a portable Blender from an official mirror and verify its SHA-256."""
from pathlib import Path
import urllib.request,hashlib,zipfile,json

ROOT=Path(r'E:\AiAgent\XLingGame\GameFactory-3A\test_data\tools')
VERSION='4.5.13'
NAME=f'blender-{VERSION}-windows-x64'

def get(url):
    return urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'Mozilla/5.0'}),timeout=60)

def main():
    ROOT.mkdir(parents=True,exist_ok=True)
    exe=ROOT/NAME/'blender.exe'
    if exe.is_file(): print(str(exe));return
    base='https://mirror.blender.org/release/Blender4.5/'
    sums=get(base+f'blender-{VERSION}.sha256').read().decode()
    expected=next(line.split()[0] for line in sums.splitlines() if NAME+'.zip' in line)
    archive=ROOT/(NAME+'.zip')
    if not archive.is_file() or hashlib.sha256(archive.read_bytes()).hexdigest()!=expected:
        with get(base+NAME+'.zip') as response, archive.open('wb') as out:
            while block:=response.read(8*1024*1024):out.write(block)
    actual=hashlib.sha256(archive.read_bytes()).hexdigest()
    if actual!=expected:raise RuntimeError('Portable Blender SHA256 mismatch')
    with zipfile.ZipFile(archive) as z:z.extractall(ROOT)
    (ROOT/'blender_toolchain.json').write_text(json.dumps({'version':VERSION,'source':base+NAME+'.zip','sha256':actual,'executable':str(exe)},indent=2),encoding='utf-8')
    print(str(exe),flush=True)

if __name__=='__main__':main()
