#!/bin/sh

export BASE_DIR="`dirname $0`"

if test x"$NO_MAKE" != x"yes"; then
    make -C $BASE_DIR/../ > /dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="$BASE_DIR/`make -s -C $BASE_DIR echo-cutter`"
fi

export CUT_UI_MODULE_DIR=$BASE_DIR/../cutter/module/ui/.libs
export CUT_RUNNER_FACTORY_MODULE_DIR=$BASE_DIR/../cutter/module/runner-factory/.libs

CUTTER_ARGS="--multi-thread -s $BASE_DIR"
if test x"$USE_GTK" = x"yes"; then
        CUTTER_ARGS="-r gtk $CUTTER_ARGS"
else
        CUTTER_ARGS="--color=auto $CUTTER_ARGS"
fi
$CUTTER $CUTTER_ARGS "$@" $BASE_DIR
