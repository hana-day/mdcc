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

echo OK
