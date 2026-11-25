#!/usr/bin/env python3
"""Print the entries of the NVS partition (0x9000, 0x5000) of an ESP32 full flash dump.
Usage: python tools/nvs_dump.py backup.bin"""
import sys, struct
d=open(sys.argv[1],'rb').read()[0x9000:0x9000+0x5000]
ns={0:'(ns)'}; out=[]
for p in range(0,len(d),4096):
    page=d[p:p+4096]
    state=struct.unpack('<I',page[:4])[0]
    if state==0xffffffff: continue
    bitmap=page[32:64]
    i=0
    while i<126:
        st=(bitmap[i//4]>>((i%4)*2))&3
        e=page[64+i*32:64+(i+1)*32]
        if st!=2: i+=1; continue  # 2 = written
        nsi,typ,span,chunk=e[0],e[1],e[2],e[3]
        key=e[8:24].split(b'\0')[0].decode('latin1')
        data=e[24:32]
        if span==0 or span>126: span=1
        if typ==0x01: v=data[0]
        elif typ==0x11: v=struct.unpack('<b',data[:1])[0]
        elif typ==0x02: v=struct.unpack('<H',data[:2])[0]
        elif typ==0x12: v=struct.unpack('<h',data[:2])[0]
        elif typ==0x04: v=struct.unpack('<I',data[:4])[0]
        elif typ==0x14: v=struct.unpack('<i',data[:4])[0]
        elif typ in (0x08,0x18): v=struct.unpack('<q' if typ==0x18 else '<Q',data)[0]
        elif typ in (0x21,0x42):
            ln=struct.unpack('<H',data[:2])[0]
            raw=page[64+(i+1)*32:64+(i+1)*32+ln]
            v=raw.rstrip(b'\0').decode('latin1') if typ==0x21 else raw.hex()[:80]+('...' if ln>40 else '')
        elif typ==0x48: v='(blob index)'
        else: v=f'type {typ:#x} {data.hex()}'
        if nsi==0: ns[v if isinstance(v,int) else 0]=key
        else: out.append((ns.get(nsi,nsi),key,typ,v))
        i+=span
for n,k,t,v in out: print(f"{n:14s} {k:16s} {v}")
