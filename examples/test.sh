#!/usr/bin/env bash

DIR="$(dirname $(realpath ${BASH_SOURCE[0]}))"

cd "${DIR}"

function cleanup() {
    sudo rmmod example_dpusm_need_provider_user
    sudo rmmod example_dpusm_no_provider_user
    sudo rmmod example_gpl_dpusm_provider
    sudo rmmod example_bsd_dpusm_provider
    sudo rmmod dpusm
}

trap cleanup EXIT

# clean up at start
cleanup || true

set -e

make -C ..
make

sudo insmod ../dpusm.ko

# this user connects to the DPUSM without ever using a provider
sudo insmod users/no_provider/example_dpusm_no_provider_user.ko

# attempt to load user module that needs a
# provider immediately, without the provider
sudo insmod users/need_provider/example_dpusm_need_provider_user.ko || true

# load the user after the provider
sudo insmod providers/bsd/example_bsd_dpusm_provider.ko
sudo insmod providers/gpl/example_gpl_dpusm_provider.ko
sudo insmod users/need_provider/example_dpusm_need_provider_user.ko

echo "Success"
# clean up will be called
