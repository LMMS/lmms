\page CodingConventions Coding Conventions

\tableofcontents

Everybody has their own preferred style for writing code, however, in the interest of making the codebase easier to maintain, we request that the following conventions be observed. Code that is lifted from other projects does not need to be modified to match these rules – no need to fix what isn't broken.

This convention page follows the requirements keywords of [RFC 2119](https://www.ietf.org/rfc/rfc2119.txt). Requirements are seen as CAPITAL letter words.

The majority of LMMS conventions follows the [Qt Coding Style](https://wiki.qt.io/Qt_Coding_Style). Some conventions are repeated here, and if not noted here MAY fall under the Qt style. The [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) also contain good advice, and MAY be referenced in situations not covered by these conventions.

## Header Files and Include Statements

### Header Guards
Header guards MUST NOT begin with an underscore `_`. Identifiers that begin with an underscore followed by a CAPITAL letter are reserved identifiers in C++. If you edit an older file which contains an improper header guard, please fix it to comply with guidelines.

Header guards should also prepend the namespace of the class the file declares. For example:

```cpp
// SongEditor.h
#ifndef LMMS_GUI_SONG_EDITOR_H
#define LMMS_GUI_SONG_EDITOR_H

namespace lmms::gui
{

class SongEditor;

} // namespace lmms::gui

#endif // LMMS_GUI_SONG_EDITOR_H

```

### Include Order
The first `#include` in a `cpp` file MUST be its own related header file. Afterwards group `#include`s by type, where groups are separated by a newline, and ordered by name.
```cpp
// MySourceFile.cpp
#include "MySourceFile.h"

#include <algorithm>
#include <vector>

#include <QMap>
#include <QString>

#include "DataFile.h"
#include "Engine.h"
#include "GuiApplication.h"
```

## Naming

### Type Names
Types MUST begin with an uppercase letter, unless a lowercase letter is required for the code to function.
```cpp
class ResourcesDB;
enum class MyEnum
{
	// ...
};
using AutoModelList = std::vector<AutomatableModel*>;

struct MyCustomDeleter
{
	using pointer = int; // Okay, since unique_ptr requires this specific type name
	void operator()(int);
};
using MyCustomPointer = std::unique_ptr<int, MyCustomDeleter>;
```

### Variable Names
Variables MUST use camel case format, for example: `nextValue`.

### Private/Protected Data Members
1. Non-static member variables MUST be prefixed with `m_`
1. Static member variables MUST be prefixed with `s_`

The remaining variable name MUST follow the Variable Names rule.
```cpp
float m_currentValue;
static int s_quantization;
```

### Public Data Members
The exposure of data members MUST be minimized by choosing more restrictive access specifiers and providing getters/setters if needed.

Exceptions:
- Public non-static member variables of POD structs with no member functions (except perhaps constructors) which are only ever accessed in a way which makes their member status obvious. These variables SHALL use camel case format in order to limit visual clutter, for example: `sampleRate`.
- Public static constants. Public static constants SHALL use `UpperCamelCase`.

```cpp
struct MyPair
{
	static constexpr int MyPublicConstant = 123; // GOOD
	static constexpr int s_myPublicConstant = 123; // BAD
	int firstMember; // GOOD
private:
	int secondMember; // BAD
};
```

### Global Constants
Global scope constants SHALL use `UpperCamelCase`.

### Function Parameters
Some older code prefixed function parameters with an underscore `_`. Function parameters SHOULD NOT be prefixed with an underscore.

### Namespaces
Namespaces MUST use `lower_snake_case`.

## Formatting

### Line Length
Every line of text SHOULD be at most 120 characters long.

### Indentation
Tabs MUST be used for indentation. Instructions for configuring QtCreator can be [found here](https://github.com/LMMS/lmms/pull/2033#issuecomment-98895801).

#### Switch Statements

Switch `case` and `default` labels within switch statements MUST be indented one level further than the `switch` keyword itself. The code within each case or default block MUST be indented one level further than the label.

```cpp
switch (dayOfWeek)
{
	case 1:
		std::cout << "Monday" << std::endl;
		break;
	default:
		// ...
}
```

### Flow Control Statements
Flow control statements (`if`, `else if`, `else`, `for`, `do`, `while`, and `switch`) MUST have a space between the keyword and the opening parenthesis.

### True/False
You MUST use the `true`/`false` keywords instead of non-standard macros or integers.
```cpp
b = TRUE; // BAD
b = true; // GOOD
b = 1; // BAD
```

### Null pointers
You MUST use the `nullptr` keyword instead of macros or integers.
```cpp
p = NULL; // BAD
p = nullptr; // GOOD
p = 0; // BAD
```

### Ternary Operator
The ternary operator `? :` SHOULD be used only when conditionally selecting one of two values. It SHOULD NOT be used when the value of the expression is not used.
```cpp
// BAD - result begins uninitialised and cannot be made const. Ample opportunity for bugs.
int value;
if (useA()) { value = getA(); }
else { value = getB(); }

// GOOD - can initialize the result variable at declaration and make it const.
const auto value = useA() ? getA() : getB();

// BAD - result is unused, code is unclear
hasError() ? throw error{"oh no"} : doSomethingElse();

// GOOD - clear
if (hasError()) { throw error{"oh no"}; }
doSomethingElse();
```

### Member Initializer Lists

Member initializer lists in constructors MUST place the `:` and `,` delimiters on the same line as the member they precede, after any indentation, separated from that member by a single space.

Member initializer lists SHOULD prefer using direct list initialization through curly braces `{}` rather than parentheses `()`, as the latter can result in silent and unintended narrowing conversions.

Non-static data member initialization ("NSDMI") SHOULD be used where possible instead of member initializer lists in constructors, especially in cases where a constructor would be empty except for a member initializer list.

```cpp
// BAD
struct BreaksAllTheRules
{
	Foo() :
		foo(123),
		bar(45.6f)
	{}

	int foo;
	float bar;
};

// BETTER
struct UsesProperInitList
{
	Foo()
		: foo{123}
		, bar{45.6f}
	{}

	int foo;
	float bar;
};

// BEST
struct UsesNSDMI
{
	int foo = 123;
	float bar = 45.6f;
};
```

### Whitespace
#### Braces
1. Spaces MUST NOT be added after an opening parenthesis or bracket
1. Spaces MUST NOT be added before a closing parenthesis or bracket
1. Flow control statements (`if`, `else if`, `else`, `for`, `do`, `while`, and `switch`) MUST use explicit blocking
1. Block braces SHOULD be on their own line
```cpp
// Spaces before/after parentheses and brackets
void doThis( int a ); // BAD
void doThis(int a); // GOOD
if ( m_sample > 0 ) // BAD
if (m_sample > 0) // GOOD
array[ offset ] // BAD
array[offset] // GOOD

// BAD - Example of not explicit blocking
if (m_sample > 0)
	--m_sample;
// GOOD - Example of explicit blocking
if (m_sample > 0)
{
	--m_sample;
}
// If the block can fit on one line, it is acceptable to format it as shown below.
// (Note that rules 1 and 2 do not apply, the contents are separated by a space on either side.)
if (m_sample > 0) { --m_sample; }
```

#### Return Statements
Return statements SHOULD NOT have parentheses around the returned value.
```cpp
return (bar); // BAD
return bar; // GOOD
```

#### Semicolons
Spaces MUST NOT be added before semicolons.
```cpp
return this ; // BAD
return this; // GOOD
```

#### Colons
Colons MUST be surrounded by spaces in range-based for loops and in base class specifications. Colons MUST NOT be preceeded by space when following a label or access specifier.
```cpp
class Foo: public Bar // BAD
class Foo : public Bar // GOOD
{
public : // BAD
public: // GOOD
	void foo()
	{
		for (const auto bar: m_baz) // BAD
		for (const auto bar : m_baz) // GOOD
		{
			switch (bar)
			{
			default : // BAD
			default: // GOOD
				break;
			}
		}
	}
};
```

#### Infix Operators
Spaces MUST be used before and after infix operators (`=`, `+`, `-`, `*`, `/`, etc.).
```cpp
sub_note_key_base=base_note_key+octave_cnt*NOTES_PER_OCTAVE; // BAD
sub_note_key_base = base_note_key + octave_cnt * NOTES_PER_OCTAVE; // GOOD
```

When wrapping long expressions, infix operators SHOULD be placed at the beginning of the line, not the end.
```cpp
someVariable = someLongNameThatMeansTheLineNeedsWrapping -
	someOtherRatherLongName +
	iCantThinkOfWhatToCallThisOne; // BAD
someVariable = someLongNameThatMeansTheLineNeedsWrapping
	- someOtherRatherLongName
	+ iCantThinkOfWhatToCallThisOne; // GOOD
```

#### Pointers
The `*` symbol in pointer declarations MUST be adjacent to the type, not the variable name.
This rule also applies to `**`, `&`.

```cpp
int* myPointer; // GOOD
int *myPointer; // BAD
int * myPointer; // BAD

MainApplication(int& argc, char** argv); // GOOD
MainApplication(int &argc, char **argv); // BAD
```

#### Ternary Operator
If long ternary expressions don't fit on one line, they MUST be formatted as one of the below, depending on the readability of the surrounding code. If the expressions are very long or convoluted, you SHOULD use if/else blocks instead.
```cpp
a == condition
	? value
	: otherValue;
// OR, depending on surrounding code readability
a == condition
? value
: otherValue;
```

## Doxygen comments

### Doxygen layout

Single-line doxygen comments SHOULD use `//! ...` for comments about the following line, and `//!< ...` for comments about the current line. Multi-line doxygen comments SHOULD use `/** ... */`.

```cpp
/**
	this
	is a
	larger
	comment
	about `class S`
*/
class S
{
	//! this comment is above `f` and explains what `f` does
	void f();
	int m_i; //!< this comment comes after `m_i` and explains what `m_i` does
};
```

For referencing code, you can use backticks, like above, or e.g. `@p` to reference a parameter name.

### When to use doxygen comments

Doxygen comments SHOULD be used when writing documentation that is relevant when using the documented entity elsewhere in the codebase. Examples include: the purpose of a function and what its parameters mean, what the data in a member variable means, what a particular class represents, or what sort of functions are contained in a namespace.

Doxygen comments SHOULD NOT be used for comments that are only relevant in the context of the code that contains them. Examples include: what the purpose of a local variable in a function is, details of how a particular algorithm works, or a note indicating that a piece of code needs refactoring.

Most comments in headers will be candidates for doxygen-style comments.

### Converting existing comments to doxygen

When submitting a PR, normal comments relevant to the modified code should be turned into doxygen comments if appropriate. For example, if you see code like

```cpp
// This function does ...
void function();
```
then change it to
```cpp
//! This function does ...
void function();
```

However, changes must be done with care: in the following examples, blindly replacing `//` by `//!` would lead to bad doxygen comments:

```cpp
// the next two functions do ...
void f();
void g();
```

```cpp
// This class could need some cleanup
class c { /* ... */ };
```

## TODO Comments

Sometimes it's worth merging code even if there are improvements to be made. In these cases its helpful to leave comments highlighting what improvements should be made and where. For the sake of discoverability, please use a searchable keyword in these comments. Some keywords currently used in the codebase are `TODO`, `HACK`, `Workaround`, and `FIXME`. An example from [PR #5427](https://github.com/LMMS/lmms/pull/5427): `// TODO: We should do this work outside the event thread`.

### Qt Versions

If a piece of code is only relevant for some versions of Qt, `QT_VERSION` and `QT_VERSION_CHECK(MAJ, MIN, PAT)` can be used to exclude it in other versions. This has the added benefit that when we update our minimum Qt version, a simple search for `QT_VERSION` can be used to remove outdated code. A simplified example based on [PR #5777](https://github.com/LMMS/lmms/pull/5777):

```
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 2)
	// Your code here
#endif
```

### C++ Versions

Changes to the C++ standard can simplify or otherwise improve existing code. Since we lag behind the standards, sometimes we can know ahead of time that a particular section of code could be better written if a newer standard were used. In this case, a comment along the lines of `// TODO C++##:` should be added. An example from [PR #5589](https://github.com/LMMS/lmms/pull/5589): `// TODO C++17 and above: use std::optional instead`
