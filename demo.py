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


# Derivation Path = 44'/304'/0'/0/0
DerivationPath = bytes("052c00008030010080000000800000000000000000".decode('hex'))
# HRP = "io"
HRP = bytes("696f".decode('hex'))

## Get public key 44'/304'/{account}0'/0/{index}0
print "Get public key ...\n"
publickey = dongle.exchange(bytes("55010000".decode('hex')) + chr(len(DerivationPath)) + DerivationPath)
print "public key: \n" + str(publickey).encode('hex')

## Get address 44'/304'/{account}0'/0/{index}0
print "Get Address ...\n"
address = dongle.exchange(bytes("55040000".decode('hex')) + chr(len(HRP) + len(DerivationPath) +1) 
		+ chr(len(HRP)) + HRP + DerivationPath)
print "public key(short)\n" + str(address[:32]).encode('hex')
print "address:\n" + address[33:]
## sign key 44'/304'/{account}0'/0/{index}0
print "\n sign request (1 of 2 package):...\n"
initsign = dongle.exchange(bytes("5502010215052c00008030010080000000800000000000000000".decode('hex')))

print "sign request (2 of 2 package):...\n"

trantosign = bytes("0801100118c09a0c2201315296010a033130301229696f3134356d766e677861736a70366473733878333877323864396c3772357766647638796c7277781a6400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000".decode('hex'))
#trantosign = bytes("0801100118a08d06220d3130303030303030303030303052400a13313030303030303030303030303030303030301229696f313837777a703038766e686a6a706b79646e723937716c68386b683064706b6b797466616d386a".decode('hex'))
apdu = bytes("55020202".decode('hex')) + chr(len(trantosign)) + trantosign
signature = dongle.exchange(apdu)
print "signature resp: \n" + str(signature).encode('hex')

sys.exit()
