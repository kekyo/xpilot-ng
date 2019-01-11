#!/bin/sh
#     Script to test the unicity of ship shapes !
#
# TkXpilot-3.2 includes the ship shapes :
# The format of the ship shapes file (TkXpi.shp) should be :
#
#   Ship shape name number 1 : (..)(..)...(...)
# You can't use the ":" character in the ship shape name.
# You can add some commented lines but the 1st character must be "#"
# You can add some empty lines (to separate the groups of shapes)
#
# Use this script each time you add a new ship shape.
#
# This test concerns only the ship shapes ! not the ship shapes names, 
# I mean : 2 ship shapes can have the same name.
#
# Warning ! TAB is the variable which contains the <Tabulation> character
#
TAB="	"
if [ $1 ]; then
	echo "Testing $1 ..."
        sed -e "s/$TAB//g" $1 | sed -e "s/ //g" > tstshpe.tmp2
        echo "The shipe shapes where \":\" is missing are :"
        grep ")(" tstshpe.tmp2 | grep -v :
        grep : tstshpe.tmp2 | sed -e "s/:/ /g" | grep ")(" | sort  -k2 > tstshpe.tmp1

        uniq -f1 tstshpe.tmp1 > tstshpe.tmp2
        echo "The redundant shapes are :"
        diff tstshpe.tmp1 tstshpe.tmp2 | sed -e "s/ / : /g"

        NB1=`wc -l tstshpe.tmp1 | cut -d\  -f1 `
        NB2=`wc -l tstshpe.tmp2 | cut -d\  -f1 `
        rm -f  tstshpe.tmp1 tstshpe.tmp2 
        echo " $NB2/$NB1 uniq ship shapes tested"
else
        echo "Usage : uniqshap ship_shape_file"
fi
