# --------------------------------------------------------------------------------
# Ethan Berg
# ECEN 5613 - Fall 2025 - Prof. McClure
# University of Colorado Boulder
# Modified Tue Nov 25 2025
# --------------------------------------------------------------------------------
# Utility script to ensure hex record is suitable for writing to 8051 bootloader
# Splits any lines that fall over a 128-byte page border
# --------------------------------------------------------------------------------

import argparse


def split_hex_record(line, split_at):

  byte_count = int(line[1:3], 16)
  address = line[3:7]
  record_type = line[7:9]
  data = line[9:9 + byte_count * 2]
  checksum = line[9 + byte_count * 2: 9 + byte_count * 2 + 2]

  if split_at >= byte_count or split_at <= 0:
    return (line, None)

  # First part
  first_byte_count = split_at
  first_data = data[:first_byte_count * 2]
  first_line_wo_checksum = (
    f":{first_byte_count:02X}{address}{record_type}{first_data}"
  )
  first_checksum = calc_hex_checksum(first_line_wo_checksum)
  first_line = first_line_wo_checksum + first_checksum

  # Second part
  second_byte_count = byte_count - split_at
  second_address = f"{int(address, 16) + split_at:04X}"
  second_data = data[first_byte_count * 2:]
  second_line_wo_checksum = (
    f":{second_byte_count:02X}{second_address}{record_type}{second_data}"
  )
  second_checksum = calc_hex_checksum(second_line_wo_checksum)
  second_line = second_line_wo_checksum + second_checksum

  return (first_line, second_line)


def calc_hex_checksum(line_wo_checksum):
  """
  Calculate the checksum for an Intel HEX line (without the starting ':' and checksum).
  Input: line_wo_checksum is the line without the starting ':' and without the checksum.
  Returns: 2-character uppercase hex string.
  """
  # Remove starting ':'
  if line_wo_checksum.startswith(':'):
    line_wo_checksum = line_wo_checksum[1:]
  # Convert to bytes
  bytes_list = [int(line_wo_checksum[i:i+2], 16) for i in range(0, len(line_wo_checksum), 2)]
  checksum = (-(sum(bytes_list)) & 0xFF)
  return f"{checksum:02X}"


def clean_hex_record(inputFile, outputFile):
    result_lines = []
    with open(inputFile, 'r') as f:
        lines = f.readlines()
      
    for line in lines:
        line = line.strip()
        if line == None:
            print("skipping none line")
            continue

        start = int(line[3:7], 16)
        len = int(line[1:3], 16)
        end = start + len

        if start//0x80 != end//0x80:
            first, line = split_hex_record(line, 0x80-start%0x80)
            if line == None:
              continue
            result_lines.append(first+"\n")
        
        result_lines.append(line+"\n")
    with open(outputFile, 'w') as f:
        f.writelines(result_lines)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Clean intel hex record to remove lines over chunk boundarise")
    parser.add_argument("i", default="build/lab3.hex", help="input file")

    args = parser.parse_args()

    clean_hex_record(args.i, args.i)