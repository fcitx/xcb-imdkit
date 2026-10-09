#!/bin/sh

XVFB=$1
READY=$2
shift 2

DISPLAY_FILE=$(mktemp) || exit 1
if [ -z "$DISPLAY_FILE" ] || [ ! -f "$DISPLAY_FILE" ]; then
    exit 1
fi

"$XVFB" -displayfd 3 -nolisten tcp 3>"$DISPLAY_FILE" &
XVFB_PID=$!

finish()
{
    kill "$XVFB_PID" >/dev/null 2>&1
    wait "$XVFB_PID" 2>/dev/null
    rm -f "$DISPLAY_FILE"
}

trap finish EXIT

DISPLAY_NUMBER=
i=1
while [ "$i" -le 5 ]; do
    if [ -s "$DISPLAY_FILE" ]; then
        IFS= read -r DISPLAY_NUMBER < "$DISPLAY_FILE"
        break
    fi
    sleep "$i"
    i=$((i + 1))
done

if [ -z "$DISPLAY_NUMBER" ]; then
    exit 1
fi
DISPLAY=:"$DISPLAY_NUMBER"

i=1
while [ "$i" -le 5 ]; do
    if DISPLAY="$DISPLAY" "$READY"; then
        break
    fi
    sleep "$i"
    i=$((i + 1))
done

if [ "$i" -gt 5 ]; then
    exit 1
fi

DISPLAY="$DISPLAY" "$@"
