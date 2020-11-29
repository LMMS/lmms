#!/usr/bin/env bash

set -e

if [ "$TYPE" != 'style' ]; then
	ccache -s
fi
