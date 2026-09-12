// test_pathprogram_builder.h

#pragma once

#include "pathprogram_builder.h"

#include <cstdio>

namespace waavs
{
    struct PathProgramPrintSink
    {
        size_t commandCount{ 0 };

        bool onMoveTo(float x, float y) noexcept
        {
            std::printf("  MOVETO  %.2f %.2f\n", x, y);
            ++commandCount;
            return true;
        }

        bool onLineTo(float x, float y) noexcept
        {
            std::printf("  LINETO  %.2f %.2f\n", x, y);
            ++commandCount;
            return true;
        }

        bool onQuadTo(float x1, float y1, float x, float y) noexcept
        {
            std::printf("  QUADTO  %.2f %.2f  %.2f %.2f\n", x1, y1, x, y);
            ++commandCount;
            return true;
        }

        bool onCubicTo(float x1, float y1, float x2, float y2, float x, float y) noexcept
        {
            std::printf("  CUBICTO %.2f %.2f  %.2f %.2f  %.2f %.2f\n", x1, y1, x2, y2, x, y);
            ++commandCount;
            return true;
        }

        bool onArcTo(float rx, float ry, float rotation, float largeArc, float sweep, float x, float y) noexcept
        {
            std::printf("  ARCTO   %.2f %.2f %.2f %.0f %.0f %.2f %.2f\n", rx, ry, rotation, largeArc, sweep, x, y);
            ++commandCount;
            return true;
        }

        bool onClose() noexcept
        {
            std::printf("  CLOSE\n");
            ++commandCount;
            return true;
        }

        bool onEnd() noexcept
        {
            std::printf("  END\n");
            ++commandCount;
            return true;
        }
    };


    static bool testPathProgramBuilder()
    {
        PathProgramBuilder builder;

        if (!builder.moveTo(10.0f, 20.0f)) return false;
        if (!builder.lineTo(30.0f, 40.0f)) return false;
        if (!builder.quadTo(50.0f, 60.0f, 70.0f, 80.0f)) return false;
        if (!builder.cubicTo(90.0f, 100.0f, 110.0f, 120.0f, 130.0f, 140.0f)) return false;
        if (!builder.arcTo(25.0f, 15.0f, 30.0f, 0.0f, 1.0f, 160.0f, 170.0f)) return false;
        if (!builder.close()) return false;
        if (!builder.finalize()) return false;

        std::printf("PathProgramBuilder:\n");

        PathProgramPrintSink printer;
        if (!pathprogram_dispatch(builder.prog, printer)) return false;

        const bool pass =
            builder.prog.ops.size() == 7 &&
            builder.prog.args.size() == 21 &&
            builder.prog.ops.back() == OP_END &&
            printer.commandCount == 7;

        std::printf("  Ops:      %zu\n", builder.prog.ops.size());
        std::printf("  Args:     %zu\n", builder.prog.args.size());
        std::printf("  Commands: %zu\n", printer.commandCount);
        std::printf("PathProgramBuilder: %s\n", pass ? "PASS" : "FAIL");

        return pass;
    }
}