#!/bin/sh
cat /etc/*-rel* | grep DISTRIB_DESCRIPTION | awk -F= '{ print $2 }' | tr -d "\" "
