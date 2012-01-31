Caml Shell
==========

Introduction
------------

Camlsh is a camllight shell with line edition features.

Requirement
-----------

Camlsh require GNU readline library (and camllight)

Compiling
---------

    wget "http://caml.inria.fr/pub/distrib/caml-light-0.75/cl75unix.tar.gz"
    tar -xvf cl75unix.tar.gz
    cd ./cl75/src/
    make configure world
    cd ../../
    make

Running
-------

    ./camlsh -stdlib /usr/lib/caml-light
