# This script modfiy TRUSTZONE_IMG_PROTECT_CFG.ini AUTH_PARAM_N and AUTH_PARAM_D items when customer set they own keys

import os
import sys

TARGET_PLATFORM = sys.argv[1]
MTEE_IMG_KEY_FILE = "./teeloader/include/mtee_key.pem"
TRUSTZONE_IMG_PROTECT_CFG_FILE = "./mtee/build/cfg/" + TARGET_PLATFORM + "/TRUSTZONE_IMG_PROTECT_CFG.ini"
TRUSTZONE_IMG_PROTECT_CFG_FILE_TEMP = "./mtee/build/cfg/" + TARGET_PLATFORM + "/TRUSTZONE_IMG_PROTECT_CFG_TEMP.ini"

def parse_key(keyfile):
	temp_key_file = "%s.temp" % keyfile
	cmdline = "openssl rsa -text -in %s -pubout > %s" % (keyfile, temp_key_file)
	os.system(cmdline)
	#check if keyfile generated success
	if not os.path.exists(temp_key_file):
		print "[error] command line excute failed: %s , please check you linux environment" % cmdline
		exit(-1)

	#parse keys
	#read keyfile
	lines = ""
	for line in open(temp_key_file):
		lines += line

	#replace \r \next
	lines = lines.replace("\n", "")
	lines = lines.replace(" ", "")

	#get public key
	#from modulus to publicExponent
	start_idx = lines.find("modulus:") + len("modulus:")
	end_idx = lines.find("publicExponent")
	public_key = lines[start_idx:end_idx]
	if public_key[:2] == "00":
		public_key = public_key[2:]
	public_key = public_key.replace(":", "")
	#print "public_key :" + public_key

	#get private key
	#from privateExponent to prime1
	start_idx = lines.find("privateExponent:") + len("privateExponent:")
	end_idx = lines.find("prime1")
	private_key = lines[start_idx:end_idx]
	if private_key[:2] == "00":
		private_key = private_key[2:]
	private_key = private_key.replace(":", "")
	#print "private_key:" + private_key

	os.remove(temp_key_file)

	return (public_key, private_key)

#check paramters
if len(sys.argv) < 2:
	print "miss parameter"
	print "usage: python cus_mtee_enc_key.py platform(ex: mt2701)\n"

#generate temp file used to modify original config
if os.path.exists(TRUSTZONE_IMG_PROTECT_CFG_FILE_TEMP):
	os.remove(TRUSTZONE_IMG_PROTECT_CFG_FILE_TEMP)
cmd = "cp -f " + TRUSTZONE_IMG_PROTECT_CFG_FILE + " " + TRUSTZONE_IMG_PROTECT_CFG_FILE_TEMP
os.system(cmd)

#modify AUTH_PARAM_N and AUTH_PARAM_D
(img_auth_public_key, img_auth_private_key) = parse_key(MTEE_IMG_KEY_FILE)
img_auth_key_list = open(TRUSTZONE_IMG_PROTECT_CFG_FILE_TEMP).readlines()
tmp = open(TRUSTZONE_IMG_PROTECT_CFG_FILE, "w+")
for i in range(0, len(img_auth_key_list)):
	if img_auth_key_list[i].startswith("AUTH_PARAM_N"):
		img_auth_key_list[i] = "AUTH_PARAM_N = 0x" + img_auth_public_key + "\n"
		tmp.write(img_auth_key_list[i])
	elif img_auth_key_list[i].startswith("AUTH_PARAM_D"):
		img_auth_key_list[i] = "AUTH_PARAM_D = 0x" + img_auth_private_key + "\n"
		tmp.write(img_auth_key_list[i])
	else:
		tmp.write(img_auth_key_list[i])

#job done, remove temp file
os.remove(TRUSTZONE_IMG_PROTECT_CFG_FILE_TEMP)