#!/usr/bin/env python

import ecc_bch
import sys
import os
import struct
import argparse

def gen_header(nand_name, out_image):
	""" device info:
	name interface pagesize(B) cycle sparesize(B) pageperblock total_blocks ecc_strength
	"""
	with open(os.getcwd() + "/tools/nand-utils/nand_device_list_v12.txt", "r") as f:
		found = 0
		for l in f:
			s = l.split()
			if s[0] == nand_name:
				found = 1
				interface = eval(s[1])
				page_size = int(s[2])
				address_cycle = int(s[3])
				spare_size = int(s[4])
				page_per_block = int(s[5])
				total_blocks = int(s[6])
				ecc_strength = int(s[7])

		if not found:
			raise KeyError("not support " + nand_name)

	# step1: generate nand header info #
	header = "BOOTLOADER!" + '\0'
	header += "V006"
	header += "NFIINFO" + '\0'
	header += struct.pack("H", interface)
	header += struct.pack("H", page_size)
	header += struct.pack("H", address_cycle)
	header += struct.pack("H", spare_size)
	header += struct.pack("H", page_per_block)
	header += struct.pack("H", total_blocks)
	header += '\0' * 24
	# defualt not use ahb mode 
	header += struct.pack("H", 0)
	header += struct.pack("B", ecc_strength)
	# fdmecc size usually is 1
	header += struct.pack("B", 1)
	header += '\0' * 16
	# calculate header ecc parity #
	header_size = 80
	ecc_level = 24
	ecc = ecc_bch.bch_enc_14(header, header_size, ecc_level)
	header += ecc
	header += '\0' * 6
	# double nfb header #
	header += header

	with open(out_image, "wb") as f:
		f.write(header)

def main(argv):
	parser = argparse.ArgumentParser()
	parser.add_argument('nand_name', help = 'nand device name')
	parser.add_argument('out_image', help = 'the output header file')
	args = parser.parse_args()

	# generate nand device header #
	gen_header(args.nand_name, args.out_image)

if __name__ == "__main__":
	sys.exit(main(sys.argv))
