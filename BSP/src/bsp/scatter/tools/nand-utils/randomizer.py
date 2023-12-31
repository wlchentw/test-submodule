#!/usr/bin/env python

import sys
import os
import argparse
import pp_tbl

SS_SEED = (
	0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7)

def fetch_seed(ran_type, seeds, idx):
	if ran_type == "SS":
		seed_page = seeds[idx % len(seeds)]
		seed_page &= 0x7fff
		key = seed_page
		return (key, seed_page)
	else:
		pass

def pn_gen(ran_type, src_len, key):
	pn_buf = []
	for i in range(src_len):
		pn_buf.append(0)
		if ran_type == "SS":
			for j in range(8):
				if bool(key & 0x4000) != bool(key & 0x2000):
					key <<= 1
					key &= 0x7ffe
					key += 1
					pn_buf[i] += 1 << j
				else:
					key <<= 1
					key &= 0x7ffe
		else:
			pass
	return (key, pn_buf)

def randomizer(in_image, out_image, ran_type, sec_reseed, page_size, spare_size, sector_size, page_per_block, ppen, vendor):
	# check file length #
	if (os.path.getsize(in_image) % (page_size + spare_size)) != 0:
		raise Exception("input image size is not page size aligned")

	page_number = os.path.getsize(in_image) / page_size
	sec_per_page = page_size / sector_size
	spare_per_sec = spare_size / sec_per_page

	if ran_type == "SS":
		seeds = SS_SEED
	else:
		raise Exception("not support %s randomizer" % ran_type)
	if ppen == 1:
		if vendor == 'TSB':
			PP = pp_tbl.TSB_PP
		else:
			raise Exception("unsupport %s pp table " % vendor)

	f_in = open(in_image, "rb")
	f_out = open(out_image, "wb")

	for i in range(page_number):
		percent = float(i) / float(page_number) * 100
		sys.stdout.writelines("%.1f"%percent)
		sys.stdout.writelines("%\r")
		sys.stdout.flush();
		if ppen == 1:
			seed_idx = PP[i % (page_per_block / 2)]
		else:
			seed_idx = i % page_per_block
		(key, seed_page) = fetch_seed(ran_type, seeds, seed_idx)
		for j in range(sec_per_page):
			if sec_reseed:
				key = seed_page

			(key, pn_buf) = pn_gen(ran_type, sector_size + spare_per_sec, key)

			in_buf = f_in.read(sector_size + spare_per_sec)
			if not in_buf:
				break
			in_buf = bytearray(in_buf)

			for k in range(sector_size + spare_per_sec):
				in_buf[k] ^= pn_buf[k]

			in_buf = str(in_buf)
			f_out.write(in_buf)

	f_in.close()
	f_out.close()

def main(argv):
	parser = argparse.ArgumentParser()
	parser.add_argument('in_image', help = 'input image w/o random ')
	parser.add_argument('out_image', help = 'output image w/ random ')
	parser.add_argument('ran_type', default = 'SS', help = 'randomizer algorithm: SS or TSB')
	parser.add_argument('sec_reseed', default = '1', help = 'whether reseed each sector')
	parser.add_argument('page_size', help = 'page size')
	parser.add_argument('spare_size', help = 'page spare size')
	parser.add_argument('sector_size', help = 'sector size')
	parser.add_argument('page_per_block', help = 'page per block number')
	parser.add_argument('ppen', help = 'whether paired page is enabled')
	parser.add_argument('vendor', help = 'vendor name')
	args = parser.parse_args()

	randomizer(args.in_image, args.out_image, args.ran_type, int(args.sec_reseed), int(args.page_size), \
			int(args.spare_size), int(args.sector_size), int(args.page_per_block), int(args.ppen), args.vendor)

if __name__ == "__main__":
	sys.exit(main(sys.argv))
