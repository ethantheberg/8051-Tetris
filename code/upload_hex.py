# --------------------------------------------------------------------------------
# Ethan Berg
# ECEN 5613 - Fall 2025 - Prof. McClure
# University of Colorado Boulder
# Modified Tue Nov 25 2025
# --------------------------------------------------------------------------------
# Utility script to write hex record to 8051 bootloader. 
# --------------------------------------------------------------------------------

import serial
import time
import sys
import argparse
import shutil
from tqdm import tqdm


def upload_hex(port, baud, filename):
    """Upload Intel HEX file to a bootloader over serial."""
    ser = serial.Serial(port, baud)
    print(f"Opened {port} at {baud} baud.")

    ser.write("U".encode("ascii"))
    assert ser.read(1) == "U".encode("ascii")

    with open(filename, "r") as f:
        lines = f.readlines()

    i = 0
    t_2 = time.perf_counter()
    for line in tqdm(lines):
        line = line.strip()
        if not line:
            continue
        i += 1

        # t_0 = time.perf_counter()
        # print(f"other elapsed={(t_0 - t_2)*1000:.3f}ms")

        ser.write(line.encode("ascii"))
        # ser.flush()

        # t_1 = time.perf_counter()
        
        resp = ser.read_until()
        
        # t_2 = time.perf_counter()
        # print(f"write elapsed={(t_1 - t_0)*1000:.3f}ms")
        # print(f"read_until elapsed={(t_2 - t_1)*1000:.3f}ms")


        return_code = chr(resp.strip()[-1])
        if return_code == ".":
            # print(f"successfully wrote line")
            continue
        elif return_code == "X":
            print("CHECKSUM ERROR")
            raise Exception()
        else:
            print(f"unknown response. Sent {line} and recieved {resp}")
    ser.close()


def backup_file(filename):
    """Copy file to filename_old, overwriting if it exists."""
    dst = f"{filename}_old"
    shutil.copy2(filename, dst)
    return dst

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Upload Intel HEX to a serial bootloader."
    )
    parser.add_argument("port", help="Serial port (e.g. /dev/ttyUSB0)")
    parser.add_argument("file", help="Intel HEX file to upload")
    parser.add_argument(
        "--baud", type=int, default=57600, help="Baud rate (default: 57600)"
    )

    args = parser.parse_args()

    upload_hex(port=args.port, baud=args.baud, filename=args.file)
