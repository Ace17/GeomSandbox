# GeomSandbox

Authors: Sebastien Alaiwan, Vivien Bonnet

Description
-----------

This is a sandbox for prototyping computational geometry algorithms.
It uses fibers to allow visual single-stepping through algorithms,
while keeping intrusivity minimal.

It uses SDL2.

Gallery
-------

Triangulation using the Bowyer-Watson algorithm:

<p align="center">
   <img src="doc/bowyerwatson.png">
   <img src="doc/bowyerwatson.gif">
</p>

Voronoi diagram using the Fortune algorithm:

<p align="center">
   <img src="doc/fortune.png">
   <img src="doc/fortune.gif">
</p>

Polyline simplification using the Douglas-Peucker algorithm:

<p align="center"><img src="doc/douglaspeucker.gif"></p>

Continuous collision detection using the separating-axis-theorem:

<p align="center"><img src="doc/app_sat.png" width="50%"></p>

Build
-----

Requirements:
```
* libsdl2-dev
```

It can be compiled to native code using your native compiler (gcc or clang):

```
$ make
```

The binaries will be generated to a 'bin' directory
(This can be overriden using the BIN makefile variable).

Run the sandbox
---------------

Just run the following command:

```
$ bin/GeomSandbox.exe <appName>
```

For example:

```
$ bin/GeomSandbox.exe Example
```

Keys:
* F2 : reset the algorithm with new input data.
* Space: single-step the current algorithm.
* Return: finish the current algorithm.
* Keypad +/- : zoom/dezoom
* Keypad arrows : scroll
