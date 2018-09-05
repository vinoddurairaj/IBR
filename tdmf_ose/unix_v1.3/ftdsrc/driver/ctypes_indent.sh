#!/bin/sh

cat - |\
	/port/ftd/portability/ctypes/ctypes |\
	/port/ftd/portability/indent/indent-1.9.1/indent -st -gnu 
