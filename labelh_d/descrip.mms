#
#   @(#)descrip.mms	1.2 03/12/31
#
#   OpenVMS MMS "Makefile" for LabelH
#
#   Contributing author: Hartmut Becker
#
#   See the COPYRIGHT file for information about the LabelH widget.
#
cc= cc
cflags= /stand=rel/extern=strict/include=("../")

all :	liblabelh(LABELH)
	! done

liblabelh(LABELH) :	LABELH.OBJ

LABELH.OBJ :	LABELH.C
LABELH.OBJ :	LABELHP.H
LABELH.OBJ :	   LABELH.H
