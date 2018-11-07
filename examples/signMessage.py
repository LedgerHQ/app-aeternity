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
from base import Request, sendApdu
import argparse
import struct

parser = argparse.ArgumentParser()
parser.add_argument('--acc', help="Account number to sign with", default=0)
parser.add_argument('--message', help="Message to be sign",
                    default="test message")
args = parser.parse_args()

accNumber = struct.pack(">I", int(args.acc))
messageLength = struct.pack(">I", len(args.message))
result = sendApdu(Request['SignMessage'], Request['NoneVerify'],
                  accNumber, messageLength, args.message)

signature = result[0: 1 + 32 + 32]
print "Signature " + ''.join(format(x, '02x') for x in signature)
