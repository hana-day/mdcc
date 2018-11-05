#!/bin/bash

test() {
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

test 0 0
test 10 10
test 3 "1 + 2"
test 6 "3 * 2"
test 2 "4 / 2"
test 14 " 3*2 + 2*4"
test 6 "(1+2) * (5-3)"

echo OK
