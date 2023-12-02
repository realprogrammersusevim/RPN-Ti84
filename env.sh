#!/bin/bash

# Small hack to detect if script is running in interactive shell
if (( SECONDS == 0 )); then
    echo -e "This script must be sourced in order to set environment variables.\nCall \`source $0\` instead."
fi

export CEDEV="$HOME/CEdev/bin"
export PATH="$CEDEV:$PATH"
