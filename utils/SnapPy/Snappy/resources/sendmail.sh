#!/bin/bash

MAILFROM="$(whoami)@met.no"
MAILTO="$(whoami)@met.no"
SUBJECT="SNAP-beregning"

FILES_TO_ADD=""

for FILE in *.png; do
    if [ ! -e $FILE ]; then
        echo -e "Argument $FILE is not a file"
        exit 1
    fi

    FILES_TO_ADD="$FILES_TO_ADD -a $FILE"
done

echo "Se vedlagt fil(er)." | mail $FILES_TO_ADD -r "$MAILFROM" -s "$SUBJECT" "$MAILTO"
