from urllib.request import urlopen
import sys

url = "https://cdn.statically.io/gh/AppImage/pkg2appimage/master/excludelist"
output = urlopen(url).read().decode("utf-8")
libs = []

for lib in output.splitlines():
    if not lib.startswith("#"):
        pl = lib.split("#")[0]
        if pl != "":
            libs.append(pl.strip())

sys.stdout.write(';'.join(libs))