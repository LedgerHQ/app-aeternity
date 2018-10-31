# æternity ledger æpp [![Build Status](https://api.travis-ci.org/aeternity/ledger-app.svg?branch=master)](https://api.travis-ci.org/aeternity/ledger-app)
## Overview
æternity wallet application framework for Ledger Nano S.
## Building and installing
To build and install the app on your Ledger Nano S you must set up the Ledger Nano S build environments. Please follow the Getting Started instructions at [here](https://ledger.readthedocs.io/en/latest/userspace/getting_started.html).

Compile and load the app onto the device:

```bash
make load
```
Remove the app from the device:
```bash
make delete
```
## Load pre-compiled HEX file
Install dependencies:
```bash
pip install ledgerblue
```
Load [latest release](https://github.com/aeternity/ledger-app/releases/latest) into device using `DATA_SIZE_OF_PRECOMPILED_HEX` and `VERSION` without `v`
```bash
python -m ledgerblue.loadApp                        \
    --curve ed25519                                 \
    --tlv                                           \
    --targetId 0x31100003                           \
    --delete --fileName app.hex                     \
    --appName "Aeternity"                           \
    --appVersion VERSION                            \
    --dataSize DATA_SIZE_OF_PRECOMPILED_HEX         \
    --path "44'/457'"                               \
    --appFlags 0x840                                \
    --icon 0100000000ffffff00ffffffffffffffffffffc7e393c9799c7d8679fc93c9c7e3ffffffffffffffff
```
Replace `VERSION` with the version of realese, for example `0.1.1`.

Replace `DATA_SIZE_OF_PRECOMPILED_HEX` with the data size of the pre-compiled HEX file that is available in release.
## Examples of Ledger wallet functionality
Install dependencies:
```bash
cd examples
pip install -r requirements.txt
```
Test functionality:
```bash
python getAddress.py --acc 0
```
## Documentation
This follows the specification available in the `api.asc`.
