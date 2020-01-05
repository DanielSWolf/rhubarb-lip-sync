"""Generates coeffs.bin file from one of the three coefficient files that come with libsamplerate."""

from re import search, findall
import struct
from array import array

# 'fastest_coeffs.h', 'mid_qual_coeffs.h', or 'high_qual_coeffs.h'
input_file_name = 'mid_qual_coeffs.h'

coeffs = None
with open(input_file_name, 'r') as input_file:
    content = input_file.read()

    # Read increment
    increment = int(search(r"=\s*\{\s*(\d+)", content).group(1))

    # Read coefficients
    coeffs = [float(coeffString) for coeffString in findall(r"-?\d+\.\d+e[-+]\d+", content)]
    # ...add a final zero coefficient
    coeffs.append(0.0)

with open('coeffs.bin', 'wb') as output_file:
    output_file.write(struct.pack('i', increment))
    array('f', coeffs).tofile(output_file)
