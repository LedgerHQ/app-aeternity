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
- Download and extract `tar.gz` file from the [latest release](https://github.com/aeternity/ledger-app/releases/latest)
- Run the `load.sh` file
- Allow unknown manager
- Validate the app `Identifier` on the Ledger Nano S with the given `Application full hash` that is available in the release info
- Confirm app installation on ledger
- Enter PIN code

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
This follows the specification available in the [`api.asc`](https://github.com/aeternity/ledger-app/blob/master/api.asc).
