#!/usr/bin/env python
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
from secp256k1 import PublicKey
import time
import sys

fake_pb_data = {
    "tx1":  "0801100118c09a0c2201315296010a033130301229696f3134356d766e677861736a70366473733878333877323864396c377235"
            "7766647638796c7277781a6400000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000",

    "tx2": "0801100118a08d06220d3130303030303030303030303052400a13313030303030303030303030303030303030301229696f31383"
           "7777a703038766e686a6a706b79646e723937716c68386b683064706b6b797466616d386a",

    "tx3": "0801100318a08d06220d3130303030303030303030303052350a0831303030303030301229696f313365736c6d306165366d64726"
           "a32757a3763323630616a363730776b6479777461796533676b",

    # """
    # 1. StakeCreate
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # candidateName: "testbot",
    # stakedAmount: "5",
    # stakedDuration: 6,
    # autoStake: true,
    # payload: "hello"
    # """
    "StakeCreate": "080110021803220134c202160a0774657374626f74120135180620012a0485e965a0",

    # """
    # 2. StakeUnstake
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # bucketIndex: 10,
    # payload: "world"
    # """
    "StakeUnstake": "080110021803220134ca0208080a1204c28ae574",

    # """
    # 3. StakeWithdraw
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # bucketIndex: 10,
    # payload: "world"
    # """
    "StakeWithdraw": "080110021803220134d20208080a1204c28ae574",

    # """
    # 4. StakeAddDeposit
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # bucketIndex: 10,
    # amount: "200",
    # payload: "world"
    # """
    "StakeAddDeposit": "080110021803220134da020d080a12033230301a04c28ae574",

    # """
    # 5. StakeRestake
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # bucketIndex: 10,
    # stakedDuration: 20,
    # autoStake: true,
    # payload: "world"
    # """
    "StakeRestake": "080110021803220134e2020c080a101418012204c28ae574",
  
    # """
    # 6. StakeChangeCandidate
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # bucketIndex: 10,
    # candidateName: "testbot",
    # payload: "world"
    # """
    "StakeChangeCandidate": "080110021803220134ea0211080a120774657374626f741a04c28ae574",

    # """
    # 7. StakeTransferOwnership
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # bucketIndex: 10,
    # voterAddress: "io24jyvf4stclr80nmgx9hrkdr0c4hptfwl7ljxdz",
    # payload: "world"
    # """
    "StakeTransferOwnership": "080110021803220134f20233080a1229696f32346a797666347374636c7238306e6d67783968726b"
                              "647230633468707466776c376c6a78647a1a04c28ae574",

    # """
    # 8. CandidateRegister
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # candidate: {
    #   name: "testbot",
    #   operatorAddress: "io14jyvf4stclr80nmgx9hrkdr0c4hptfwl7ljxdz",
    #   rewardAddress: "io54jyvf4stclr80nmgx9hrkdr0c4hptfwl7ljxdz"
    # },
    # stakedAmount: "1000",
    # stakedDuration: 30,
    # autoStake: false,
    # ownerAddress: "io24jyvf4stclr80nmgx9hrkdr0c4hptfwl7ljxdz",
    # payload: "world"
    # """
    "CandidateRegister": "080110021803220134fa029a010a5f0a0774657374626f741229696f31346a797666347374636c7238306e6d"
                         "67783968726b647230633468707466776c376c6a78647a1a29696f35346a797666347374636c7238306e6d67"
                         "783968726b647230633468707466776c376c6a78647a120431303030181e2a29696f32346a79766634737463"
                         "6c7238306e6d67783968726b647230633468707466776c376c6a78647a3204c28ae574",

    # """
    # 9. CandidateUpdate
    # version=1,nonce=2,gasLimit=3,gasPrice=4
    # name: "testbot",
    # operatorAddress: "io14jyvf4stclr80nmgx9hrkdr0c4hptfwl7ljxdz",
    # rewardAddress: "io54jyvf4stclr80nmgx9hrkdr0c4hptfwl7ljxdz"
    # """
    "CandidateUpdate": "08011002180322013482035f0a0774657374626f741229696f31346a797666347374636c7238306e6d677839687"
                       "26b647230633468707466776c376c6a78647a1a29696f35346a797666347374636c7238306e6d67783968726b64"
                       "7230633468707466776c376c6a78647a"
}


def sign(trantosign):
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

    # Action name or raw data
    signature = dongle.exchange(bytearray.fromhex("55020202") + bytes([len(trantosign)]) + trantosign)
    print("signature resp: {}".format(signature.hex()))


if __name__ == "__main__":
    if len(sys.argv) > 1:
        if sys.argv[1] in fake_pb_data:
            action = sys.argv[1]
            print("Action: {}".format(action))
            sign(bytearray.fromhex(fake_pb_data.get(action)))
        else:
            sign(bytearray.fromhex(sys.argv[1]))
    else:
        print("Please select action:\n\t{}".format("\n\t".join(sorted(fake_pb_data.keys()))))
        sys.exit(0)
