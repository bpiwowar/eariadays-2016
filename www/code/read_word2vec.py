import struct
import gzip

with gzip.open("data/GoogleNews-vectors-negative300.bin.gz", "rb") as f:
    words, dim = f.readline().split()
    words = int(words)
    dim = int(dim)
    st = struct.Struct("%df" % dim)
    print("Dimension", dim)

    for i in range(words):
        s=b''
        while True:
            c=f.read(1)
            if c == b' ': 
                break
            s += c

        s = str(s, encoding='utf-8')
        x = st.unpack(f.read(st.size))
