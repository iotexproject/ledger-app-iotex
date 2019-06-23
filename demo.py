#!/usr/bin/env python

from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
from secp256k1 import PublicKey
import sys

## Get version
print "Get Version ...\n"
dongle = getDongle(True)
osversion = dongle.exchange(bytes("E001000000".decode('hex')))
print "os version:" + str(osversion).encode('hex')


## Get public key 44'/304'/{account}0'/0/{index}0
print "Get public key ...\n"
publickey = dongle.exchange(bytes("5501000015052c00008030010080000000800000000000000000".decode('hex')))
print "public key: \n" + str(publickey).encode('hex')

## Get address 44'/304'/{account}0'/0/{index}0
print "Get Address ...\n"
address = dongle.exchange(bytes("550400001802696f052c00008030010080000000800000000000000000".decode('hex')))
print "public key(32 bytes) + Address: \n" + str(address).encode('hex')

## sign key 44'/304'/{account}0'/0/{index}0
print "sign test init...\n"
initsign = dongle.exchange(bytes("5502010211052c00008030010080000000800000000000000000".decode('hex')))
print "initsign resp: \n" + str(initsign).encode('hex')

print "sign send...\n"

trantosign = bytes("0801100118c09a0c2201315296010a033130301229696f3134356d766e677861736a70366473733878333877323864396c3772357766647638796c7277781a6400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000".decode('hex'))
apdu = bytes("55020202".decode('hex')) + chr(len(trantosign)) + trantosign
signature = dongle.exchange(apdu)
print "signature resp: \n" + str(signature).encode('hex')

sys.exit()
textToSign = ""
while True:
	data = raw_input("Enter text to sign, end with an empty line : ")
	if len(data) == 0:
		break
	textToSign += data + "\n"

dongle = getDongle(True)
publicKey = dongle.exchange(bytes("8004000000".decode('hex')))
print "publicKey " + str(publicKey).encode('hex')
try:
	offset = 0
	while offset <> len(textToSign):
		if (len(textToSign) - offset) > 255:
			chunk = textToSign[offset : offset + 255] 
		else:
			chunk = textToSign[offset:]
		if (offset + len(chunk)) == len(textToSign):
			p1 = 0x80
		else:
			p1 = 0x00
		apdu = bytes("8002".decode('hex')) + chr(p1) + chr(0x00) + chr(len(chunk)) + bytes(chunk)
		signature = dongle.exchange(apdu)
		offset += len(chunk)  	
	print "signature " + str(signature).encode('hex')
	publicKey = PublicKey(bytes(publicKey), raw=True)
	signature = publicKey.ecdsa_deserialize(bytes(signature))
	print "verified " + str(publicKey.ecdsa_verify(bytes(textToSign), signature))
except CommException as comm:
	if comm.sw == 0x6985:
		print "Aborted by user"
	else:
		print "Invalid status " + comm.sw 

