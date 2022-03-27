---
id: ljyjdguu3ffvsa74kgiu2de
title: ""
desc: 'the documentation for suugu'
updated: 1646433176143
created: 1646433176143
---

<style>
    
    * {
        background-color:rgb(17, 20, 26);
        color:rgb(163, 159, 159);
    }
    h1{
        text-align:center;
    }
    h2{
        text-decoration: underline;
        text-decoration-thickness: 1px;
    }
    table{
        border-bottom:solid;
        border-width:1px;
    }
    table {
        padding:2px 5px;
        width:100%
    }
    a{
        color:rgb(98, 151, 255)
    }

    .container{
        width:fit-content;
        padding:4px;
        border-style:double;
        border-width: 3px;
        border-top-color: rgb(80, 87, 128);
        border-left-color: rgb(80, 87, 128);
        border-right-color: rgba(0,0,0,0);
        border-bottom-color: rgba(0,0,0,0);
    }
    .indent{
        margin-left:10px;
    }
</style>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Title)
# suugu

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Table of Contents)
<details> <summary style="font-size:1.2em;">Table of Contents</summary>
<div class="indent">

[[Input|suugu#Input]]  
[[Solving|suugu#Solving]]  
[[Graphing|suugu#Graphing]]  
[[Workspace|suugu#Workspace]]  
[[Serialization|suugu#Serialization]]   

</div>
</details>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Terminology)
## Terminology
<div class="indent">

|<div style=width:100px>term</div>|definition|
|---|---|
term       | generic base thing (literal, operator, variable, function call, etc)  
variable   | unknown number represented by a single letter  
literal    | number or string literally representing itself  
token      | character(s) that represent a term in text form  
operator   | symbol that represents an operation on one or many terms  
expression | collection of terms in the form of a syntax tree  
equation   | expression with an explicit equals operator  
workspace  | region of the canvas in which expressions are able to interact together  
graph      | graphing grid with a local camera in which equations can be drawn  
element    | anything with position, size, coordinate space, and display info  
canvas     | manages the elements, world, and input

&#9679; NOTE: In AST code comments, literals/lit might refer to literal, variable, or function call, since they all behave the same.
</div>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Input)
## Input
### Methods for inputting math 
##### types.h, canvas.h, canvas.cpp
<div class="indent">
<details class="container"> <summary>todos</summary>

- [ ] simple math input (operators)
    - [x] literals
    - [ ] variables
    - [x] addition/subtraction
    - [x] multiplication/division
    - [ ] parenthesis
    - [ ] exponentials
	- [x] equals
- [ ] complex math input
    - [ ] implicit multiplication between variables (and constants)
    - [ ] variable super/sub script
	- [ ] inequalities
    - [ ] integrals/derivatives
    - [ ] limits
    - [ ] function notation
    - [ ] sum
    - [ ] patterns
    - [ ] vectors/matrices
    - [ ] ± (plus or minus)
- [ ] hotstrings (turn into symbols or place slots in non-linear ways)
    - [ ] matMN  expands to a MxN matrix eg mat22 expands to an empty 2x2 matrix
    - [ ] vecN   expands to a vector with N values
    - [ ] sumNI  expands to a sum starting at n = N to i = I (ideally with a way to change the sum var)
    - [ ] prodNI same concept but with product
    - [ ] intI   expands to an indefinite integral of a given expression
    - [ ] intAB  expands to a definite integral evaluated from A to B of a given expression
    - [ ] ddX    expands to a derivative of a given expression of variable X
    - [ ] pdX    expands to a partial derivative of a given expression of variable X
    - [ ] sqrtN  expands to a square root to the power of N
	- [ ] logN   expands to a log of base N
	- [ ] abs    expands to absolute value
    - [ ] deg    expands to degrees symbol
- [ ] builtin functions (argument based functions like coding)
    - [ ] sin/cos/tan (inverse/hyperbolic versions)
    - [ ] ceil/floor/round
	- [ ] min/max
	- [ ] logarithms
- [ ] expression assignment
- [ ] symbols menu
- [ ] ability to quickly switch between math and programmer inputs
- [ ] TeX input (and other math syntax)
- [ ] drag-n-drop elements of an expression
- [ ] box select on matrices to grab out rows/columns/sub-matrices
- [ ] button to move the view of a graph to an expression
- [ ] mouse-expression interactions
    - [x] single left click:        select element and deselect any previously selected (keybinds target selected)
    - [ ] single shift left click:  select another element (multiple select)
    - [ ] single shift right click: deselect element
    - [ ] double left click:        start editing the clicked element like text (might also start an expression)
    - [ ] single right click:       shows simple context menu on clicked element (might also start an expression)
    - [ ] single shift right click: show meta context menu on clicked element (change looks, format, extract to text, color, label, etc)
    - [ ] single ctrl right click:  show detailed context menu on clicked element (with search for all options)
    - [ ] left hold:                move selected elements, affecting the expression
    - [ ] shift left hold:          copy selected elements and allows dragging onto other elements
- [ ] other keybinds
    - [ ] Alt-S   : Attempt to simplify statement
    - [ ] Alt-G   : Graph statement in a new graph window  
        3D if we detect 2 independent variables  
        2D if we detect 1  
        If we detect more than 2 then probably warn  
        or treat it as an implicit 3D function  
    - [ ] Alt-D   : Attempt to solve equation for a var  
        this would query for a variable to solve for  
        0 would rearrange the equation to = 0  
    - [ ] =       : moves between 2 sides of an equation  
        we will probably have 2 different cursors for each  
        side, and if the user moves one cursor to the other  
        side through the equals sign, we move both  
    - [ ] Shft->
    - [ ] Shft-< : move an expression to the other side maybe  
        for example if you select idk  
        "-3" in the equation  
        "2x-3=0"  
        and press Shft-> it will make the equation  
        "2x=3"  
    - [ ] Ctrl->
    - [ ] Ctrl-< : move between statements on the canvas
</details>
</div>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Parsing)
## Parsing
### Lexing and parsing the math input into an AST
##### types.h, lexer.cpp, parser.cpp
<div class="indent">

&#9679; The exponent operator in programmer input is `**`.

<details class="container"> <summary>todos</summary>

- [ ] simple math
    - [ ] literals
    - [ ] variables
    - [ ] addition/subtraction
    - [ ] multiplication/division
    - [ ] parenthesis
    - [ ] exponentials
	- [ ] equals/assignment
- [ ] complex math
    - [ ] implicit multiplication between variables (and constants)
    - [ ] variable super/sub script
	- [ ] inequalities
    - [ ] integrals/derivatives
    - [ ] limits
    - [ ] function notation
    - [ ] sum
    - [ ] patterns (python colon, c for, ellipses)
    - [ ] vectors/matrices
    - [ ] ± (plus or minus)
- [ ] programmer math
    - [ ] literals
    - [ ] variables
    - [ ] addition/subtraction
    - [ ] multiplication/division
    - [ ] parenthesis
    - [ ] exponentials
	- [ ] equals/assignment
- [ ] hotstrings (turn into symbols or place slots in non-linear ways)
- [ ] functions (argument based functions like coding)
</details>
</div>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Solving)
## Solving
### Solving the AST received from Parsing
##### types.h, solver.cpp
<div class="indent">

<details> <summary>It might be confusing to define variables with the = operators, so we could use : instead.</summary>

```js
a: 2x + 3 = 4y + 6
b: 5x

c: a + b
c is 2x + 3 + 5x = 4y + 6 + 5x
```
</details>

&#9679; Solving should be on a different thread so that it doesnt interrupt input/rendering.  
&#9679; Expressions are evaluated into an AST as you type.  
&#9679; Automatic solving if = is present with nothing to one side.  

<details class="container"> <summary>todos</summary>

- [x] addition/subtraction
- [x] multiplication/division
- [ ] exponentials
- [ ] inequalities
- [ ] integrals/derivatives
- [ ] limits
- [ ] differential equations
- [ ] sum
- [ ] patterns
- [ ] vectors/matrices
- [ ] ± (plus or minus)
- [ ] functions
- [ ] units solving
</details>
</div>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Graphing)
## Graphing
<div class="indent">

&#9679; Ability to name/label variables, so if you hover over them it shows their full name.  
&#9679; Visualizing the value of a variable where it is used during an animation so say you have cos(a) and a is changing during an animation it will show in place of a a's actual value.  

<details class="container"> <summary>todos</summary>

- [ ] 2D graphing
    - [ ] linear equations
    - [ ] non-linear equations
    - [ ] piece-wise equations
    - [ ] parametric equations
    - [ ] inequality shading (desmos)
    - [ ] scalar fields
    - [ ] vector fields
    - [ ] contour plots
- [ ] 3D graphing
    - [ ] lines
    - [ ] surfaces
    - [ ] volumes
    - [ ] vector fields
    - [ ] scalar fields?
- [ ] Link graphing
    - [ ] trees
    - [ ] nodegraphs
</details>
</div>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Workspace)
## Workspace
<div class="indent">
<details class="container"> <summary>todos</summary>

- [ ] scoping a workspace with a box
- [ ] free draw workspace that acts like a mix between paint and math tools where it keeps all math input functionality inside the workspace
- [ ] workspace variable tracking window
- [ ] option to view rational decimals as fractions
- [ ] a library of constants similar to what SpeedCrunch has
</details>
</div>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// Serialization)
## Serialization
### Saving, Loading, Importing, Exporting, and Config
<div class="indent">
<details class="container"> <summary>todos</summary>

- [ ] recording of input actions and playback/stepthru that can be serialized and sent to others
- [ ] export-to-text for programming languages supports different variable sizes
- [ ] row vs column major matrices
- [ ] +z forward vs backward
- [ ] left vs right handed
</details>
</div>

[//]: # (///////////////////////////////////////////////////////////////////////////////////////// References)
## References
<div class="indent">

https://www.desmos.com/calculator  
https://www.geogebra.org/calculator  
https://www.geogebra.org/3d  
https://www.symbolab.com/  
https://www.wolframalpha.com/  
https://mathworld.wolfram.com/
</div>