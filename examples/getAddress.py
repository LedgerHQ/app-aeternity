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
parser.add_argument('--acc', help="Account number to retrieve", default=0)
args = parser.parse_args()

accNumber = struct.pack(">I", int(args.acc))
result = sendApdu(Request['GetAddress'], Request['Verify'], accNumber)

address = result[1 : 1 + result[0]]

print "Address " + str(address)
