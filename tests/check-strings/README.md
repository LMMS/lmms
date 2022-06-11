# Check strings

This tool checks for invalid strings not necessarily in the code.

Background: When you move a lot of classes and files, the code may be OK, but
many strings outside the code (translations, style.css, .gitmodules etc.) may
still need to be updated. This script checks whether these strings are still
matching the code.

Content:

* `check-strings`: Script to check for invalid strings
* `verify`: Script verifying `check-strings`
* expectation.txt: Expectation file for verify script

