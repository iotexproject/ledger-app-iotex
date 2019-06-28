# ledger-app-iotex
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

IoTeX wallet app according to ledger specs.


Follow this [link](https://ledger.readthedocs.io/en/latest/userspace/getting_started.html) to setup development SDK, toolchain, application loader.

## Get source
Get source code to your local workspace, by calling:
```
git clone https://github.com/iotexproject/ledger-app-iotex.git
```

## Dependencies

### Ledger Nano S

This project requires ledger firmware 1.5.5

### Ledger Python Tools

Ledger firmware 1.5.5 requires the blue-loader-python and ledgerblue from pip. Please follow the instructions here to get started:

https://github.com/LedgerHQ/blue-loader-python/tree/2a04716311c7e18236e59db3ccaa365592f2244b#python-tools-for-ledger-blue-and-nano-s

This tool requires python 2.7 - some versions do not run correctly with python 3.x versions. In order to check which version you are using run this in your terminal:
```python --version```

There are different ways of installing python 2.7 side-by-side your existing version:

**a) using anaconda**

step 1: ```conda create -n py27 python=2.7 anaconda```

step 2: ```source activate py27```

and then:

step 3: ```source deactivate```
to switch back to the original environment

**b) using virtualenv**

step1: Download python 2.7 version

step2: ```virtualenv -p {python_location} {env_name}```

step3: ```source env_name/bin/activate```

and then:

step 4: ```deactivate```
to switch back to the original environment

### Ubuntu Dependencies
Install the following packages:
```
sudo apt update && apt-get -y install build-essential git sudo wget cmake libssl-dev libgmp-dev autoconf libtool
```

### OSX Dependencies
It is recommended that you install brew and xcode. 

Additionally you will need to:


```
brew install libusb
```

## Building

To build the app, follow ALL of the instructions in the link below to get your BOLOS development environment set up.

https://ledger.readthedocs.io/en/latest/userspace/getting_started.html

Then, use the Makefile to build and load the app:

```bash
$ make
$ make load
```

## Run Demo
A short python demo script is provided to test basic function.
Connect the Ledger device and open IoTeX app, and then run the script:
```
#python demo.py
```



