#!/usr/bin/env python
"""
*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************
"""
import argparse
import struct
from base import Transaction, Request, sendApdu
from rlp import encode
from base58 import b58decode_check

parser = argparse.ArgumentParser()
parser.add_argument('--sender', help="Sender address", default="ak_2swhLkgBPeeADxVTAVCJnZLY5NZtCFiM93JxsEaMuC59euuFRQ")
parser.add_argument('--recipient', help="Recipient address", default="ak_DzELMKnSfJcfnCUZ2SbXUSxRmFYtGrWmMuKiCx68YKLH26kwc")
parser.add_argument('--amount', help="Amount to send", default=1000000000)
parser.add_argument('--fee', help="fee", default=1)
parser.add_argument('--ttl', help="ttl", default=9007199254740991)
parser.add_argument('--nonce', help="Nonce associated to the account", default=0)
parser.add_argument('--payload', help="Payload", default="")
parser.add_argument('--acc', help="Account number to sign with", default=0)
parser.add_argument('--tx', help="Hex encoded transaction")
args = parser.parse_args()

addressPrefix = '01'.decode('hex')

if args.tx != None:
    encodedTx = args.tx.decode('hex')
else:
    tx = Transaction(
        sender=addressPrefix + b58decode_check(args.sender[3:]),
        recipient=addressPrefix + b58decode_check(args.recipient[3:]),
        fee=int(args.fee),
        amount=int(args.amount),
        ttl=int(args.ttl),
        nonce=int(args.nonce),
        payload=args.payload
    )
    encodedTx = encode(tx, Transaction)

accNumber = struct.pack(">I", int(args.acc))
result = sendApdu(Request['SignTx'], Request['NoneVerify'], accNumber, encodedTx)

signature = result[0: 1 + 32 + 32]
print "Signature " + ''.join(format(x, '02x') for x in signature)
