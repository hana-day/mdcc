#!/bin/bash

test_() {
    expected="$1"
    input="$2"
    ./mdcc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"
    if [ "$actual" == "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

test_ 1 "{1;}"
test_ 3 "{1 + 2;}"
test_ 6 "{3 * 2;}"
test_ 2 "{4 / 2;}"
test_ 14 "{ 3*2 + 2*4;}"
test_ 6 "{(1+2) * (5-3);}"
test_ 3 "{ 4 / 2; 2 + 1;}"
test_ 3 "{ a = 3; a;}"
test_ 7 "{ a = 1 + 2 * 3; a;}"
test_ 2 "{ a = 1; a + 1;}"
test_ 1 "{ z(); }"
test_ 3 "{ z() * 2 + 1; }"
test_ 2 "{ a = 1; b = 2; b; }"
test_ 3 "{ test1 = 1; test2 = 2; test3 = 3; test4 = 4; test5 = 5; test3; }"
test_ 48 "{ '0'; }"
test_ 97 "{ a = 'a'; a;}"
test_ 2 "{ int a = 1; a = 2; a;}"
echo OK
