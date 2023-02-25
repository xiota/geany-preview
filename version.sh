#!/usr/bin/env bash

## @file
## @brief Generate the version number
##
## This script is called from configure.ac
##
## A revison number is generated of the form:
## <tag>.r<revision>.g<hash>


if [ -d .git ] ; then
	version=$(git describe --long --tags --always | sed -E 's/^v//;s/-([0-9]+)-/-r\1-/')
	set -- $version
	echo -n "$1"
else
	version='0.0.2'
	set -- $version
	echo -n "$1"
fi
