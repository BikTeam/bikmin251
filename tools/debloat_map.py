import sys

def remove_linker_section(buffer: str) -> str:
    keyword = ".init section layout"
    try:
        index = buffer.index(keyword)
        return buffer[index:]
    except:
        exit()

if __name__ == "__main__":
    with open(sys.argv[1], "r") as f:
        buffer: str = f.read()
    
    with open(sys.argv[1], "w") as f:
        if (buffer != ""):
            index1 = buffer.index(".init section layout")
            index2 = buffer.index("Memory map:")
            f.write(buffer[index1:index2])
