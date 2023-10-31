#!/usr/bin/env bash

VERSION="0.0.1"

if [[ $1 == "-v" ]];then
	echo "$VERSION"
	exit 1
fi


pid=`pgrep SpatGRIS`

osascript -  "$pid"  <<EOF

    on run argv -- argv is a list of strings
        tell application "System Events"
			set frontmost of every process whose unix id is (item 1 of argv) to true
        end tell
    end run

EOF