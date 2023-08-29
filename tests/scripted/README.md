# Scripted checks

The checks in this directory are not C++ unit tests, but simple scripts that
check the code for issues.

## Verify

[verify](verify) is a verification script for the scripts in this folder.

## Check strings

[check-strings](check-strings) checks for invalid strings not necessarily in
the code.

Background: When you move a lot of classes and files, the code may be OK, but
many strings outside the code (translations, style.css, .gitmodules etc.) may
still need to be updated. This script checks whether these strings are still
matching the code.

## Check namespace

[check-namespace](check-namespace) checks namespaces and a few related things
like `#ifdef`s.

