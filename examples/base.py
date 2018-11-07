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
from rlp.sedes import big_endian_int, binary, Binary
from rlp import Serializable

Request = {'NoneVerify': 0, 'Verify': 1, 'GetAddress': 2,
           'SignTx': 4, 'GetConfig': 6, 'SignMessage': 8}

address = Binary.fixed_length(33, allow_empty=True)


class Transaction(Serializable):
    fields = [
        ('type', big_endian_int),
        ('id', big_endian_int),
        ('sender', address),
        ('recipient', address),
        ('amount', big_endian_int),
        ('fee', big_endian_int),
        ('ttl', big_endian_int),
        ('nonce', big_endian_int),
        ('payload', binary)
    ]

    def __init__(self, sender, recipient, amount, fee,
                 ttl, nonce, payload, type=12, id=1):
        super(Transaction, self).__init__(type, id, sender, recipient,
                                          amount, fee, ttl, nonce, payload)


def sendApdu(*arg):
    argLength = len(arg)
    apdu = ("e00" + str(arg[0]) + "0" + str(arg[1]) + "00").decode('hex')
    if argLength > 2:
        length = 0
        data = ""
        for i in range(2, argLength):
            length += len(arg[i])
            data += arg[i]
        apdu += chr(length) + data
    dongle = getDongle(True)
    return dongle.exchange(bytes(apdu))
