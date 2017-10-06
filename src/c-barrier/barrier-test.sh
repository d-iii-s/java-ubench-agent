#!/bin/sh

BARRIER_NAME="$1"
if [ -z "$BARRIER_NAME" ]; then
    BARRIER_NAME="barr-$$"
fi

MY_HOME=`which -- "$0" 2>/dev/null`
# Maybe, we are running Bash
[ -z "$MY_HOME" ] && MY_HOME=`which -- "$BASH_SOURCE" 2>/dev/null`
MY_HOME=`dirname -- "$MY_HOME"`
MY_ROOT="$MY_HOME/../../"

run() {
    echo "[barrier-test]:" "$@"
    "$@"
}

run_barrier_test() {
	run java \
	    -cp "$MY_ROOT/out/classes/:$MY_ROOT/out/test-classes/" \
	    "-agentpath:$MY_ROOT/out/agent/libubench-agent.so" \
	    cz.cuni.mff.d3s.perf.BarrierTest "$2" "$1"
}

run "$MY_ROOT/out/barrier/ubench-barrier" "$BARRIER_NAME" 2
run_barrier_test "$BARRIER_NAME" First &
run sleep 3
run_barrier_test "$BARRIER_NAME" Second &

wait
