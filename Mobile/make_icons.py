"""Generate original opaque app icons; uses only the Python standard library."""
import json
import math
import struct
import sys
import zlib
from pathlib import Path


def png(path, size):
    rows = bytearray()
    for y in range(size):
        rows.append(0)
        for x in range(size):
            u, v = (x + .5) / size, (y + .5) / size
            glow = math.exp(-((u-.5)**2+(v-.62)**2)*9)
            color = [12+8*glow, 22+28*glow, 36+34*glow]
            # A softly rounded, illuminated jelly with a ground shadow.
            px, py = abs(u-.5), abs(v-.48)
            qx, qy = max(px-.19, 0), max(py-.18, 0)
            distance = math.hypot(qx, qy)-.115
            coverage = max(0, min(1, .5-distance*size))
            if coverage:
                edge = math.exp(-max(0, -distance)*20)
                gleam = math.exp(-((u-.37)**2+(v-.29)**2)*200)
                jelly = [48+112*gleam+35*edge, 179+55*gleam+40*edge,
                         168+68*gleam+46*edge]
                color = [a*(1-coverage)+b*coverage for a,b in zip(color,jelly)]
            rows.extend(max(0,min(255,round(c))) for c in color)
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',size,size,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))


def generate(root):
    root=Path(root)
    png(root/'res/drawable/icon.png',192)
    assets=root/'Assets.xcassets'
    assets.mkdir(parents=True,exist_ok=True)
    (assets/'Contents.json').write_text(json.dumps({'info':{'version':1,'author':'3D Jelly Physics'}}))
    app=assets/'AppIcon.appiconset'
    images=[]
    for size,scale,idiom in [(20,1,'ipad'),(20,2,'ipad'),(29,1,'ipad'),(29,2,'ipad'),(40,1,'ipad'),(40,2,'ipad'),(76,1,'ipad'),(76,2,'ipad'),(83.5,2,'ipad'),(1024,1,'ios-marketing')]:
        pixels=round(size*scale);name=f'icon-{pixels}.png';png(app/name,pixels)
        images.append({'size':f'{size:g}x{size:g}','scale':f'{scale}x','idiom':idiom,'filename':name})
    (app/'Contents.json').write_text(json.dumps({'images':images,'info':{'version':1,'author':'3D Jelly Physics'}},indent=2))


if __name__=='__main__':
    generate(sys.argv[1])
