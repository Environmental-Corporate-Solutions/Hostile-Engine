# Hostile Engine
## _Environmental Corporate Solutions_

### Build Status
[![Build-Windows-x64](https://github.com/Environmental-Corporate-Solutions/Hostile-Engine/actions/workflows/build_win_64.yml/badge.svg?branch=main)](https://github.com/Environmental-Corporate-Solutions/Hostile-Engine/actions/workflows/build_win_64.yml)

Hostile Engine is a custom ECS engine built using Flecs

## Members

The Following Names are the members of Environmental Corporate Solutions Game Team.
* Isaiah Dickson
* Sam Biks
* Junwoo Seo
* Byeongkyu Park
* Chad Glover
* Kiara Santiago 


Hostile Engine is an ECS Architectured Game Engine built using flecs


> The overarching design goal of Hostile Engine 
> is to make a Successful competitor to the Unity Game Engine
> that still brings a similar ease of use 
> to the forefront while maintaining excellent performance 
> for game developers no matter the endeavor.



## Introduction

	This document represents the architectural  style decisions of Environmental Corporate Solutions (henceforth called ECS). All members will adhere to the document in the case of obscurity or unclear instruction when designing or implementing systems or making functional changes to the code base. 


## Variables

#### Member Variables - "m_"
Hostile Engine upon refactoring shall  use “m_” prefix  for member variables.
Member variables are defined as a variable that is associated with a specific object, system, or class that can be accessed by all of the aforementioned methods.  

```cpp
class Example
{
Public:
Example(); 
Private:
	SimpleMath::Vector3 m_example; //member variable naming convention.
}
```



## Local Variables - snake_case

Local Variables shall use snake_case throughout the development of Hostile Engine.
Snake case is the style of writing in which each space is replaced with an underscore character, and the first letter of each word is written in lowercase.

```cpp
Void SomeFun()
{
Std::size_t some_variable; //snake_case local variable example
}	
```


## Function Parameters- _param

The Function Parameters Shall be  labeled with an underscore prefix “ _ “when declaring any input or output variables. A function parameter is defined as any parameter  within the signature or declaration of a function. In the case that the parameter is long or uses multiple words then snake_case shall be the appropriate convention along with the underscore prefix.

```c
Void SomeFunction(int _example, vector<int> _example_other)
{
	//in the case of this function signature _example and _example_other are both  examples of function parameters.
}
```



## Const Usage -const ref&
Const  usage  within Hostile Engine shall be as follows.
When declaring any variable reference that is const qualified. The User should put const before the variable reference. All other  usage declarations from the previous sections follow suit as needed.

```c
Void SomeFunction(const int& _example)
{
   Const reference& ref = some_reference;
 
}
```


		
## Sal Annotations -Yes
	
Sal Annotations are defined as a set of annotations that you can use to describe how a function uses its parameters. Information on SAL annotations and their usage / types can be found here.

```c
Void SomeFunction(_in_opt_ int _param)
{
    DoStuff();
}
```

## Line Length - 80

	Line Length has been declared to be no longer than 80 characters.

## Line spacing - use tabs

The defined space structure is to use Tabs with the correct spacing set to 4 space width. 
This holds true for all aspects of code in Engine. This was the agreed upon measure
During team discussion. 

## Braces - next line

Braces shall be on a new line as opposed to the google style of opening brace on the same line of declaration

## Exceptions - All allowed

Exceptions shall be allowed for use in engine. For debugging All are allowed but for code needed upon release VERIFY should be used for code that needs to be present in release build. This ensures the code being checked remains in build rather than being removed due to being associated with an assert. 

## Globals/Singletons/Namespaces

Globals - Globals shall not be used in the engine. If the need arises for a global variable declaration then the code must be redone so as to not expose any global parameters. The code will be subject to code review afterwards to ensure it meets standards.
Singletons -  Singletons are encouraged for use when the need for one arises to ensure that certain systems have only one allocation.
Namespaces - The predefined namespace is “Hostile” but others may be defined if the need arises. This allows the user to avoid ambiguous calls and conflicts with other packages used.
