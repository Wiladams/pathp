// test_pathprogram_parse.h

#pragma once

//#include "test_core.h"
#include "svg_path_reader.h"
#include "pathprogram_parse.h"
#include "pathprogram_builder.h"

#include <cstdio>

namespace waavs
{
    struct ParsePrintSink
    {
        size_t commandCount{ 0 };

        bool onMoveTo(float x, float y) noexcept
        {
            std::printf("  MOVETO   %.2f %.2f\n", x, y);
            ++commandCount;
            return true;
        }

        bool onLineTo(float x, float y) noexcept
        {
            std::printf("  LINETO   %.2f %.2f\n", x, y);
            ++commandCount;
            return true;
        }

        bool onQuadTo(float x1, float y1, float x, float y) noexcept
        {
            std::printf("  QUADTO   %.2f %.2f  %.2f %.2f\n", x1, y1, x, y);
            ++commandCount;
            return true;
        }

        bool onCubicTo(float x1, float y1, float x2, float y2, float x, float y) noexcept
        {
            std::printf("  CUBICTO  %.2f %.2f  %.2f %.2f  %.2f %.2f\n", x1, y1, x2, y2, x, y);
            ++commandCount;
            return true;
        }

        bool onArcTo(float rx, float ry, float rotation, float largeArc, float sweep, float x, float y) noexcept
        {
            std::printf("  ARCTO    %.2f %.2f %.2f %.0f %.0f  %.2f %.2f\n",
                rx, ry, rotation, largeArc, sweep, x, y);
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


    static void printRawSVGCommand(SVGPathCommand cmd, const float* args, bool repeated)
    {
        const uint8_t ch = static_cast<uint8_t>(cmd);
        const uint8_t arity = kSVGPathArity[ch];

        std::printf("  %c%s", char(ch), repeated ? " (repeated)" : "");

        for (uint8_t i = 0; i < arity; ++i)
            std::printf(" %.2f", args[i]);

        std::printf("\n");
    }


    static bool testPathProgramParse()
    {
        static const char* pathData =
            "M 10 20 30 40 "
            "l 5 6 "
            "H 50 "
            "v 10 "
            "C 60 70 80 90 100 110 "
            "s 20 30 40 50 "
            "Q 150 160 170 180 "
            "t 20 30 "
            "A 25 15 30 0 1 220 230 "
            "z";

        const MemSpan input(pathData);


        // ------------------------------------------------------------
        // Raw SVG parser
        // ------------------------------------------------------------

        std::printf("SVGPathReader:\n");

        SVGPathReader reader(input);

        SVGPathCommand cmd{};
        float args[7]{};
        bool repeated = false;
        size_t rawCommandCount = 0;

        for (;;)
        {
            const SVGPathReadResult result = reader.next(cmd, args, repeated);

            if (result == SVGPathReadResult::End)
                break;

            if (result == SVGPathReadResult::Error)
            {
                std::printf("SVGPathReader: FAIL\n");
                return false;
            }

            printRawSVGCommand(cmd, args, repeated);
            ++rawCommandCount;
        }

        if (rawCommandCount != 11)
        {
            std::printf("SVGPathReader: FAIL - expected 11 commands, got %zu\n", rawCommandCount);
            return false;
        }

        std::printf("SVGPathReader: PASS\n\n");


        // ------------------------------------------------------------
        // Parse -> normalize -> build
        // ------------------------------------------------------------

        PathProgram prog;

        if (!pathProgram_parse(input, prog))
        {
            std::printf("PathProgram parse: FAIL\n");
            return false;
        }


        // ------------------------------------------------------------
        // Dispatch resulting normalized program
        // ------------------------------------------------------------

        std::printf("Normalized PathProgram:\n");

        ParsePrintSink printer;

        if (!pathprogram_dispatch(prog, printer))
        {
            std::printf("PathProgram dispatch: FAIL\n");
            return false;
        }


        // ------------------------------------------------------------
        // Structural checks
        //
        // Raw:
        //
        //   M 10 20
        //   M 30 40       repeated -> LINETO
        //   l 5 6         -> LINETO 35 46
        //   H 50          -> LINETO 50 46
        //   v 10          -> LINETO 50 56
        //   C ...
        //   s ...         -> expanded CUBICTO
        //   Q ...
        //   t ...         -> expanded QUADTO
        //   A ...
        //   z
        //
        // Normalized:
        //
        //   MOVETO
        //   LINETO x4
        //   CUBICTO x2
        //   QUADTO x2
        //   ARCTO
        //   CLOSE
        //   END
        // ------------------------------------------------------------

        const bool pass =
            prog.ops.size() == 12 &&
            prog.args.size() == 37 &&
            prog.ops.back() == OP_END &&
            printer.commandCount == 12;

        std::printf("\n");
        std::printf("  Raw commands:        %zu\n", rawCommandCount);
        std::printf("  Program ops:         %zu\n", prog.ops.size());
        std::printf("  Program args:        %zu\n", prog.args.size());
        std::printf("  Dispatched commands: %zu\n", printer.commandCount);
        std::printf("PathProgram parser/builder: %s\n", pass ? "PASS" : "FAIL");

        return pass;
    }
}
