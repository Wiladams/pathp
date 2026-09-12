# PathProgram

**PathProgram** is a small, header-only C++17 library for parsing, normalizing, storing, and processing vector paths.

The core representation is intentionally independent of SVG or any other input format. SVG path data is provided as one frontend, but paths can also be constructed directly from font outlines, generated geometry, or other sources.

```text
SVG path data ----> SVGPathReader ----> PathCommandNormalizer --+
                                                               |
TrueType glyf -------------------------------------------------+--> PathProgramBuilder --> PathProgram
                                                               |
Generated geometry --------------------------------------------+
```

This makes `PathProgram` useful as a lightweight common representation for vector geometry.

## Features

* Header-only C++17
* Small, reusable path representation
* Pull-based SVG path parser
* SVG command normalization
* Relative coordinates converted to absolute coordinates
* Repeated `M/m` coordinates converted to line segments
* `H/V` converted to line segments
* Smooth `S/s` and `T/t` commands expanded
* Quadratic and cubic Bezier curves
* SVG elliptical arcs
* Multiple subpaths and close-path operations
* Sink-based path dispatch
* No SVG dependency in the core `PathProgram` representation
* Suitable for use with font outlines such as TrueType `glyf`

## Core Path Representation

A `PathProgram` consists of two compact streams:

```cpp
struct PathProgram
{
    std::vector<uint8_t> ops;
    std::vector<float> args;
};
```

The supported normalized operations are:

```text
MOVETO
LINETO
CUBICTO
QUADTO
ARCTO
CLOSE
END
```

The representation contains no SVG-specific commands such as relative coordinates, `H`, `V`, `S`, or `T`.

For example:

```text
m 10 20
h 50
v 25
```

is normalized into the equivalent of:

```text
MOVETO 10 20
LINETO 60 20
LINETO 60 45
```

## Parsing an SVG Path

The easiest way to turn an SVG path `d` attribute into a `PathProgram` is:

```cpp
#include "pathprogram_parse.h"

using namespace waavs;

PathProgram path;

if (!pathProgram_parse(ByteSpan("M 10 20 L 100 20 L 100 80 Z"), path))
{
    // Invalid path data
}
```

`pathProgram_parse()` performs the complete pipeline:

```text
SVG text
   |
   v
SVGPathReader
   |
   v
PathCommandNormalizer
   |
   v
PathProgramBuilder
   |
   v
PathProgram
```

## Pulling SVG Commands Directly

`SVGPathReader` can also be used independently.

It is a pull-based parser, so the caller controls when each command is read and may stop at any time.

```cpp
#include "svg_path_reader.h"

using namespace waavs;

SVGPathReader reader(ByteSpan("M 10 20 30 40 L 50 60"));

SVGPathCommand cmd{};
float args[7]{};
bool repeated = false;

for (;;)
{
    SVGPathReadResult result = reader.next(cmd, args, repeated);

    if (result == SVGPathReadResult::End)
        break;

    if (result == SVGPathReadResult::Error)
        break;

    // Process one SVG command tuple here.
}
```

For:

```text
M 10 20 30 40
```

the reader produces:

```text
M 10 20    repeated=false
M 30 40    repeated=true
```

The reader preserves raw SVG command semantics. Conversion of the repeated `M` into a line operation is performed by `PathCommandNormalizer`.

## Building a Path Directly

SVG parsing is not required.

A `PathProgram` can be built directly:

```cpp
#include "pathprogram_builder.h"

using namespace waavs;

PathProgramBuilder builder;

builder.moveTo(10.0f, 20.0f);
builder.lineTo(100.0f, 20.0f);
builder.quadTo(130.0f, 40.0f, 100.0f, 80.0f);
builder.lineTo(10.0f, 80.0f);
builder.close();
builder.end();

PathProgram path = std::move(builder.prog);
```

This is useful when the geometry already exists in another representation.

For example, a TrueType `glyf` outline can emit its contours directly:

```cpp
builder.moveTo(x, y);
builder.lineTo(x, y);
builder.quadTo(cx, cy, x, y);
builder.close();
```

No SVG parser or SVG command representation is involved.

## Processing a Path with a Sink

A `PathProgram` can be replayed into any compatible sink.

```cpp
struct PrintSink
{
    bool onMoveTo(float x, float y)
    {
        std::printf("M %g %g\n", x, y);
        return true;
    }

    bool onLineTo(float x, float y)
    {
        std::printf("L %g %g\n", x, y);
        return true;
    }

    bool onQuadTo(float x1, float y1, float x, float y)
    {
        std::printf("Q %g %g %g %g\n", x1, y1, x, y);
        return true;
    }

    bool onCubicTo(float x1, float y1, float x2, float y2, float x, float y)
    {
        std::printf("C %g %g %g %g %g %g\n", x1, y1, x2, y2, x, y);
        return true;
    }

    bool onArcTo(float rx, float ry, float rotation,
        float largeArc, float sweep, float x, float y)
    {
        std::printf("A %g %g %g %g %g %g %g\n",
            rx, ry, rotation, largeArc, sweep, x, y);
        return true;
    }

    bool onClose()
    {
        std::printf("Z\n");
        return true;
    }

    bool onEnd()
    {
        return true;
    }
};
```

Dispatch the path with:

```cpp
PrintSink sink;
pathprogram_dispatch(path, sink);
```

This sink model allows the same `PathProgram` to feed renderers, flatteners, geometry analyzers, converters, debug printers, or other processors.

## Architecture

The library deliberately separates parsing from path storage.

### `SVGPathReader`

Reads raw SVG path syntax one command tuple at a time.

```text
SVG text -> command + arguments
```

It knows SVG syntax, but knows nothing about `PathProgram`.

### `PathCommandNormalizer`

Converts raw SVG commands into canonical path operations.

It handles:

```text
relative -> absolute
H/V      -> LINETO
S/s      -> CUBICTO
T/t      -> QUADTO
repeated M/m -> LINETO
```

### `PathProgramBuilder`

Constructs a canonical `PathProgram`.

It does not know where the geometry came from.

### `PathProgram`

Stores normalized path geometry.

It knows nothing about SVG parsing or SVG command syntax.

This separation allows other geometry sources to use the same representation without depending on the SVG frontend.

## Header Overview

```text
svg_path_command.h
    Raw SVG command enumeration.

svg_path_reader.h
    Pull-based standalone SVG path syntax reader.

pathcommand_normalizer.h
    Converts SVG commands into canonical path operations.

pathprogram.h
    Core normalized path representation.

pathprogram_builder.h
    Builds and dispatches PathProgram objects.

pathprogram_parse.h
    Convenience function combining the SVG reader,
    normalizer, and builder.
```

## Example

SVG input:

```text
M 10 20 30 40
l 5 6
H 50
v 10
Q 70 80 90 100
t 20 30
Z
```

The resulting `PathProgram` is conceptually:

```text
MOVETO  10 20
LINETO  30 40
LINETO  35 46
LINETO  50 46
LINETO  50 56
QUADTO  70 80 90 100
QUADTO  110 120 110 130
CLOSE
END
```

All SVG-specific shorthand has disappeared.

## Design Goals

PathProgram favors:

* simple data structures
* explicit control flow
* low dependency count
* reusable components
* format-independent geometry
* efficient sequential storage
* easy embedding in larger libraries

The SVG parser is intentionally small enough to be included wherever SVG path parsing is needed, while the path representation itself remains useful even in applications that do not use SVG at all.

## Requirements

* C++17 or later
* Standard C++ library

No external parsing library is required.

