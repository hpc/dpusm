# Data Processing Unit Services Module (DPUSM)

The Data Processing Unit Services Module is a Linux Kernel Module that exposes a unified interface for accelerators to provide useful file system operations such as compression, checksumming, and erasure coding. Accelerators will interface with the DPUSM using the provider API to create providers. These providers will register with the DPUSM. Third parties will then use the user interface to find and use registered providers to accelerate operations.

## Build

```
cd dpusm
make
sudo insmod dpusm.ko
sudo rmmod dpusm
```

## Usage

1. Implement a provider that fills in the [provider api struct](include/dpusm/provider_api.h).
2. Load the provider and register it with the DPUSM
2. Create a user that calls the functions in the [user api](include/dpusm/user_api.h).

## [License](LICENSE)

The Data Processing Unit Services Module is dual licensed under [GPL v2](licenses/GPLv2/COPYING) and [BSD-3](licenses/BSD-3/LICENSE.txt).