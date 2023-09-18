import os
import sys

def read_dtb(filename, raw_data):
	fin = open(filename, 'rb')
	fin.seek(0, 0)
	while True:
		t_byte = fin.read(1)
		if len(t_byte) == 0:
			break
		else :
			raw_data.append("0x%.2X" % ord(t_byte))

def write_blob_head_file(filename, raw_data, length):
	fout = open(filename, 'w+')
	fout.write('#define CHECK_RSA 1 \n')
	fout.write('#define CHECK_HASH 1 \n')
	fout.write('const unsigned char blob[] __attribute__((aligned(4))) = \n')
	fout.write('{\n ')

	i = 0
	for data in raw_data:
		i += 1
		if i != length:
			fout.write(data + ', ')
		else:
			fout.write(data)
		if i % 16 == 0:
			fout.write('\n ')
		if i == length:
		       break;
	fout.write('\n};')
	fout.close()

if __name__ == "__main__":
	raw_data = []
	in_path  = str(sys.argv[1]);
	out_path = str(sys.argv[2]);
	length   = int(sys.argv[3],16);
	read_dtb(in_path, raw_data)
	write_blob_head_file(out_path, raw_data, length)
