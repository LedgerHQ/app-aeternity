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
from aeBase import Transaction, Request, sendApdu
from rlp import encode

parser = argparse.ArgumentParser()
parser.add_argument('--sender', help="Sender address")
parser.add_argument('--recipient', help="Recipient address")
parser.add_argument('--amount', help="Amount to send")
parser.add_argument('--fee', help="fee", default='1')
parser.add_argument('--ttl', help="ttl")
parser.add_argument('--nonce', help="Nonce associated to the account")
parser.add_argument('--payload', help="Payload")
parser.add_argument('--acc', help="Account number to sign with")
parser.add_argument('--tx', help="Hex encoded transaction")
args = parser.parse_args()

if args.acc == None:
    args.acc = 0

if args.sender == None:
    args.tx = "f8560c01a101f75e53f57822227a58b463095d6dab657cab804574be62de0be1f95279d09037a1011d7ce9bcf06e93c844c02489862b623c23b14a7364350a36336c87bc76b6650a843b9aca0001871fffffffffffff0780"

if args.payload == None:
    args.payload = ""

if args.tx != None:
    encodedTx = args.tx.decode('hex')
else:
    tx = Transaction(
        sender=args.sender.decode('hex'),
        recipient=args.recipient.decode('hex'),
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
