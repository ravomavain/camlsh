Caml Shell
==========

Introduction
------------

Camlsh is a camllight wrapper that aim to bring shell-like behaviours to camllight console, like for exemple the ability use 'up' and 'down' key to access previous commands, or simply the ability to go back in the current line to edit it.
Camlsh also provide an option (-o file.ml) to load file.ml in order to test its functions from command line.

Requirement
-----------

Camlsh require GNU readline library, camllight and a custom camltop.

Custom Camltop
--------------

In order to work properly, camlsh need a custom version of camltop, with modified prompts :

1.Download camllight 0.75 (never tested with other versions) :

    wget "http://caml.inria.fr/pub/distrib/caml-light-0.75/cl75unix.tar.gz"
    tar -xvf cl75unix.tar.gz

2.Edit ./src/compiler/config.mlp and replace the 2 last lines by:

    let toplevel_input_prompt = "\014";;
    let error_prompt = "\021>";;

3.Compile camllight :

    cd ./cl75/src/
    make configure
    make world

4.Your newly created camltop is in ./src/toplevel/camltop

For everything else, you can use a default camllight installation (like the one of your package manager).
I also recommand you not to install your custom camltop as your system default camltop, but rather put it in one of those locations:
    ./camltop
    ~/.camlsh/camltop
    /usr/local/lib/caml-light/camlshtop
    /usr/lib/caml-light/camlshtop

Compiling
---------

Camlsh itself can be compiled with:

    gcc -O3 -o camlsh camlsh.c -lreadline

