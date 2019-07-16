#!/bin/bash

###########################################################
# This script generates a nested Table of Contents from a #
# markdown document's header tags.                        #
# https://github.com/leo-yuriev                           #
###########################################################


# DEFAULTS
# Input .md file; e.g. README.md
INPUT=""
# Output .md file to write to; e.g. TOC.md
OUTPUT="toc.md"
# Amount of headers to skip from top of file; e.g. the file's title
SKIP=0
# How deeply the ToC should nest; "0" signifies full depth
DEPTH=0

checkFlags() {
    while [ $# -gt 0 ]; do
        case "$1" in
            -s|--skip)
                SKIP=$(( $2 + 1 )) # the skip + offset
                shift # shift past integer value for skip
                ;;
            -d|--depth)
                DEPTH=$2
                shift # shift past integer value for depth
                ;;
            *.md|*.markdown)
                if [ -z "$INPUT" ]; then
                    INPUT=$1
                else
                    OUTPUT=$1
                fi
                ;;
            -*)
                echo "Unrecognised flag; aborting..."
                return 1
                ;;
            *)
                echo "Unrecognised argument; aborting..."
                return 1
                ;;

        esac
        shift # shift all cmd line args one position left -> next arg is now $1
    done
}


# Detects the top level header type and generates a hierarchical
# list of bullet points relative to the top level.
# $1 - A list of markdown headers
generateTree() {
    HEADERS=$1

    # Log the file -> get first line -> isolate the header tag
    TL=$( LC_ALL=C sort $HEADERS|head -1|sed -r "s/(#+) (.+)/\1/" )
    SIZE=${#TL}
    INDENT="-"

    echo "Top level header: ${TL}"

    # if $DEPTH has been given, only iterate until we reach it...
    if [ $DEPTH -ne 0 ]; then
        DCOUNT=0
        while [ "$DCOUNT" -lt "$DEPTH"  ]; do
            sed -i "s/^$TL /$INDENT /" $HEADERS
            TL="$TL#"
            INDENT="    $INDENT"
            SIZE=$((SIZE + 1))
            DCOUNT=$((DCOUNT + 1))
        done
        sed -i "/^##*/d" $HEADERS # delete unaffected lines
    else
        # ...else until the smallest possible header is reached
        while [ $SIZE -lt 7 ]; do
            sed -i "s/^$TL /$INDENT /" $HEADERS
            TL="$TL#"
            INDENT="    $INDENT"
            SIZE=$((SIZE + 1))
        done
    fi
}


# The ToC generator
makeToc() {
    # Pull out all the header tags
    # then trim $SKIP lines (e.g. ``-s 2` -> repo name & `## Contents` itself)
    # then write to filename passed as $OUTPUT
    tail -n+$SKIP $INPUT | grep -E "^\#+ .+" | tr -s '[:blank:]' ' ' > $OUTPUT

    generateTree $OUTPUT

    # Turn each point into a markdown link
    ## LY: gitlab mode = all lowercase
    #sed -i -r 's/(\- )(.+ ?.*)/\1[\2](#\L\2)/' $OUTPUT
    ## LY: github mode = ascii lowercase
    sed -i -r 's/(\- )(.+ ?.*)/\1[\2](#\2)/' $OUTPUT
    sed -i ':a; s/(#\([^A-Z]*\)\([A-Z]\+\)\(.*\))$/(#\1\L\2\E\3)/g; ta' $OUTPUT

    # remove )(/ from anchor links
    sed -i -r ':a; s/\(#([^\(\)\/]*)[\(\)\/](.*)\)$/(#\1\2)/g; ta' $OUTPUT

    # Replace spaces in anchor links with hyphens
    sed -i ':a; s/(#\([^ ,"()=+/?.]*\)\([ ,"()=+/?.]\+\)\(.*\))$/(#\1-\3)/g; ta' $OUTPUT
    # Normalise monospace anchor links (remove backticks)
    sed -i -r ':a; s/\(#([^`]*)`(.*)`(.*)\)$/(#\1\2\3)/g; ta' $OUTPUT

    ## manage hypens
    # for github
    ##sed -i 's/(#-/(#/g;s/-)$/)/g' $OUTPUT
    # for legacy gitlab
    sed -i 's/(#-/(#/g;s/--\+/-/g' $OUTPUT
    ## for modern gitlab
    #sed -i 's/(#-/(#/g;s/-)$/)/g;s/--\+/-/g' $OUTPUT

#    # Log the final result written to $OUTPUT
#    cat $OUTPUT
}


#############################
# MAIN
#############################

checkFlags "$@"

if [ -z "$INPUT" ]; then
    echo "Too few arguments supplied."
    echo "Minimum: source-file.md"
    return 1
fi

makeToc

