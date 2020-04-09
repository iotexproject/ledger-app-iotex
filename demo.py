#!/usr/bin/env python
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
from secp256k1 import PublicKey
import sys

# Get version
print("Get Version ...")
dongle = getDongle(True)
osversion = dongle.exchange(bytearray.fromhex("E001000000"))
print("Os version:{}\n".format(osversion.hex()))


# Derivation Path = 44'/304'/0'/0/0
DerivationPath = bytearray.fromhex("052c00008030010080000000800000000000000000")
# HRP = "io"
HRP = bytearray.fromhex("696f")

# Get public key 44'/304'/{account}0'/0/{index}0
print("Get public key ...")
publickey = dongle.exchange(bytearray.fromhex("55010000") + bytes([len(DerivationPath)]) + DerivationPath)
print("public key: {}\n".format(publickey.hex()))

# Get address 44'/304'/{account}0'/0/{index}0
print("Get Address ...")
args_len = len(HRP) + len(DerivationPath) + 1
address = dongle.exchange(bytearray.fromhex("55040000") + bytes([args_len, len(HRP)]) + HRP + DerivationPath)

print("public key(short): {}".format(address[:32].hex()))
print("address: {}\n".format(address[33:].hex()))

## sign key 44'/304'/{account}0'/0/{index}0
print("\nSign request (1 of 2 package):...\n")
initsign = dongle.exchange(bytearray.fromhex("5502010215052c00008030010080000000800000000000000000"))
print("Sign request (2 of 2 package):...\n")

#trantosign = bytearray.fromhex("0801100118c09a0c2201315296010a033130301229696f3134356d766e677861736a70366473733878333877323864396c3772357766647638796c7277781a6400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000")
trantosign = bytearray.fromhex("0801100118a08d06220d3130303030303030303030303052400a13313030303030303030303030303030303030301229696f313837777a703038766e686a6a706b79646e723937716c68386b683064706b6b797466616d386a")
signature = dongle.exchange(bytearray.fromhex("55020202") + bytes([len(trantosign)]) + trantosign)
print("signature resp: {}".format(signature.hex()))

sys.exit()

