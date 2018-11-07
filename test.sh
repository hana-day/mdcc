#!/bin/bash

test() {
    echo "Start a test for '$input' = '$actual'"
    expected="$1"
    input="$2"
    ./mdcc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"
    if [ "$actual" != "$expected" ]; then
        echo "$input expected, but got $actual"
        exit 1
    fi
}

test 1 "{1;}"
test 3 "{1 + 2;}"
test 6 "{3 * 2;}"
test 2 "{4 / 2;}"
test 14 "{ 3*2 + 2*4;}"
test 6 "{(1+2) * (5-3);}"
test 2 "{ 4 / 2; 2;}"

echo OK
