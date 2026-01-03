import numpy

I = 0b11110000
J = 0b10001110
L = 0b00101110
O = 0b11001100
S = 0b01101100
T = 0b01001110
Z = 0b11000110

pieces = [I,J,L,O,S,T,Z]
piecenames = ["I","J","L","O","S","T","Z"]

def generateBitpack(piece):
    size = []
    shape = []
    matrix = numpy.zeros((4, 4))
    for i in range (8):
        matrix[i // 4][i % 4] = (piece >> (7-i)) & 1
    for rotation in range(4):
        matrix = alignTopLeft(matrix)
        bitstring = 0
        maxX = 0
        maxY = 0
        for y in range(4):
            for x in range(4):
                if(matrix[y][x]):
                    # print(f"block found at {x}, {y}")
                    maxX = max(maxX, x)
                    maxY = max(maxY, y)
                    bitstring <<= 4
                    bitstring |= x << 2 | y
        size.append(maxX << 2 | maxY)
        shape.append(bitstring)
        matrix = numpy.rot90(matrix, 3)
    return size, shape

def alignTopLeft(matrix):
  rows = matrix.any(axis=1)
  cols = matrix.any(axis=0)
  if not rows.any() or not cols.any():
    return matrix.copy()
  top = numpy.argmax(rows)
  left = numpy.argmax(cols)
  out = numpy.zeros_like(matrix)
  sub = matrix[top:, left:]
  rh, cw = sub.shape
  out[:rh, :cw] = sub
  return out

def printShapeDefinition():
    sizes = []
    shapes = []
    for piecename, piece in zip(piecenames, pieces):
        size, shape = generateBitpack(piece)
        sizes.append(size)
        shapes.append(shape)
    print(f"const __code uint8_t sizes[][] = {{")
    for size, name in zip(sizes, piecenames):
        print("    {", end = '')
        for bitstring, i in zip(size, range(4)):
            print(f"0b{bitstring:0{4}b}", end=', ' if i != 3 else '')
        print(f"}}, // {name}")
    print(f"}};\nconst __code uint16_t pieces[][] = {{")
    for shape, name in zip(shapes, piecenames):
        print("    {", end = '')
        for bitstring, i in zip(shape, range(4)):
            print(f"0b{bitstring:0{16}b}", end=', ' if i != 3 else '')
        print(f"}}, // {name}")
    print("};")
    
printShapeDefinition()
