# mdcc

mdcc is a small compiler for C programming language.
I wrote this just for fun and studying low-level programming.

The frontend and the backend of mdcc are written by hand.
After the abstract syntax tree is created, mdcc converts the tree into the
stack machine code written in x86-64 assembly without using IR.
Currently, there are lots of missing features such as struct, preprocessor,
global variables and etc. But, mdcc can compile relatively complex programs
like this [Brain fu*ck interpreter](https://gist.github.com/hyusuk/3c4a7ad0513a9893de40512cb2e22eae).


## Resources

- [低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook/) (by Rui Ueyama) ... The book describes how to make C compiler by incremental development.
- [9cc](https://github.com/rui314/9cc) (by Rui Ueyama) ... A readable C compiler implementation. I read the code when I stambled.
- [BNF notation for C](https://cs.wmich.edu/~gupta/teaching/cs4850/sumII06/The%20syntax%20of%20C%20in%20Backus-Naur%20form.htm)
- [ABI for x86-64](https://github.com/hjl-tools/x86-psABI)
- [C99 specification](http://www.open-std.org/JTC1/SC22/WG14/www/projects#9899)
