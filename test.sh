#!/bin/bash

test_() {
    expected="$1"
    input="$2"
    ./mdcc "$input" > tmp.s
    gcc -arch x86_64 -o tmp tmp.s
    ./tmp
    actual="$?"
    if [ "$actual" == "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

test_ 1 "int main() { return 1;}"
test_ 3 "int main() {return 1 + 2;}"
test_ 6 "int main() { return 3 * 2;}"
test_ 2 "int main() {return 4 / 2;}"
test_ 14 "int main() { return 3*2 + 2*4;}"
test_ 6 "int main() {return (1+2) * (5-3);}"
test_ 3 "int main() { 4 / 2; return 2 + 1;}"
test_ 3 "int main() { int a = 3; return a;}"
test_ 7 "int main() { int a = 1 + 2 * 3; return a;}"
test_ 2 "int main() { int a = 1; return a + 1;}"
test_ 1 "int z() { return 1; }  int main() { return z(); }"
test_ 3 "int z() { return 1; } int main() { return z() * 2 + 1; }"
test_ 2 "int main() { int a = 1; int b = 2; return b; }"
test_ 3 "int main() { int test1 = 1; int test2 = 2; int test3 = 3; int test4 = 4; int test5 = 5; return test3; }"
test_ 48 "int main() { return '0'; }"
test_ 97 "int main() { int a = 'a'; return a;}"
test_ 2 "int main() { int a = 1; a = 2; return a;}"
test_ 1 "int main() { int a = 1; int *b = &a; return *b; }"
test_ 2 "int main() { int a = 1; a *= 2; return a; }"
test_ 1 "int main() { int a = 2; a /= 2; return a; }"
test_ 2 "int main() { int a = 1; a += 1; return a; }"
test_ 1 "int main() { int a = 2; a -= 1; return a; }"
test_ 2 "int main() { int a = 1; { a = 2; } return a;}"
test_ 1 "int main() { int a = 1; { int a = 2; } return a;}"
test_ 1 "int z(int a) { return a; } int main() { return z(1); }"
echo OK
