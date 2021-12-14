#!/bin/bash                                                                                             

SPHENIXJSEBDIR=/home/cfmcginn/Projects/sPHENIXJSEB/
SPHENIXJSEBDIRLIB="$SPHENIXJSEBDIR"lib

if [[ -d $SPHENIXJSEBDIR ]]
then
    echo "SPHENIXJSEBDIR set to '$SPHENIXJSEBDIR'; if wrong please fix"
    export SPHENIXJSEBDIR=$SPHENIXJSEBDIR


    if [[ $LD_LIBRARY_PATH == *"$SPHENIXJSEBDIRLIB"* ]]
    then
        dummy=0
    else
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SPHENIXJSEBDIRLIB
    fi
else
    echo "SPHENIXJSEBDIR given, '$SPHENIXJSEBDIR' not found!!! Please fix"
fi
