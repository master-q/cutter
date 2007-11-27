#!/bin/sh

export BASE_DIR="`dirname $0`"

if test x"$NO_MAKE" != x"yes"; then
    make -C $BASE_DIR/../../ > /dev/null || exit 1
fi

export CUT_UI_MODULE_DIR=$BASE_DIR/../../cutter/module/ui/.libs
export CUT_RUNNER_FACTORY_MODULE_DIR=$BASE_DIR/../../cutter/module/runner-factory/.libs
$BASE_DIR/cutter-cairo \
    --color=auto --multi-thread -s $BASE_DIR "$@" $BASE_DIR
