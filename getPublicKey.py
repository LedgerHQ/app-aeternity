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
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
import argparse
import struct

parser = argparse.ArgumentParser()
parser.add_argument('--acc', help="Account number to retrieve")
args = parser.parse_args()

if args.acc == None:
    args.acc = 0

accNumber = struct.pack(">I", int(args.acc))
apdu = "e0020100".decode('hex') + chr(len(accNumber)) + accNumber

dongle = getDongle(True)
result = dongle.exchange(bytes(apdu))
address = result[1 : 1 + result[0]]

print "Address ak_" + str(address)
