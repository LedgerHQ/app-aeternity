# æternity ledger æpp [![Build Status](https://api.travis-ci.org/aeternity/ledger-app.svg?branch=master)](https://api.travis-ci.org/aeternity/ledger-app)
## Overview
æternity wallet application framework for Ledger Nano S.
## Building and installing
To build and install the app on your Ledger Nano S you must set up the Ledger Nano S build environments. Please follow the Getting Started instructions at [here](https://ledger.readthedocs.io/en/latest/userspace/getting_started.html).

Compile and load the app onto the device:

```
make load
```
Remove the app from the device:
```
make delete
```
## Examples of Ledger wallet functionality
Install dependencies:
```bash
$ cd examples
$ pip install -r requirements.txt
```
Test functionality:
```bash
python getAddress.py --acc 0
```
## Documentation
This follows the specification available in the `api.asc`
