GMK Perl
=========

Perl interface for GNU Make.

Requirements
------------
* GNU make compiled with the 'load' feature enabled
* GNU make header files must be installed

Compiling on GNU/Linux x86
------------
    make
    make test

Compiling on OS X
------------
    make osx=1
    make osx=1 test

An Option for Debugging perl
------------
    make PERL_DEBUG=1 test

Using the Extension
------------
`load perl.so`

Interfaces from make to Perl
------------
* `$(perl code)`
* `$(perl-load filename)`
* `$(call perl-free)`
* Any functions defined by gmk_add_function from Perl.

Interfaces from Perl to make
------------
* gmk_expand
* gmk_eval
* gmk_add_function
