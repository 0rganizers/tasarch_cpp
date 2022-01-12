import os
import sys
import coloredlogs
import logging
from pwn import *
coloredlogs.install(logging.INFO)
log = logging.getLogger("del_garbage")

def car_is_nasty(b: int):
    return b 

def identify_files(folder: bytes):
    log.info("Identifying nasty files in folder %s", folder)
    ret = []
    for f in os.listdir(folder):
        path = os.path.join(folder, f)
        if len(f) <= 8:
            nasty = False
            try:
                _ = f.decode("utf8")
            except:
                nasty = True
            if nasty:
                log.info("Found nasty %s: 0x%x", f, u64(f.ljust(8, b'\0')))
                ret.append(path)
    return ret



def main():
    files = identify_files(os.getcwd().encode("utf8"))
    for f in files:
        os.unlink(f)

if __name__ == "__main__":
    main()