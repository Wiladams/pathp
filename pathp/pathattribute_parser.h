#pragma once


#include "pathprogram_builder.h"
#include "pathsegment_normalizer.h"
#include "svg_path_reader.h"

namespace waavs
{
    static bool parsePathProgram(const ByteSpan& input, PathProgram& outProg)
    {
        SVGPathReader reader(input);
        PathProgramBuilder builder;
        SVGPathNormalizer normalizer(builder);

        SVGPathCommand cmd{};
        float args[7]{};
        bool repeated = false;

        for (;;)
        {
            const SVGPathReadResult result = reader.next(cmd, args, repeated);

            if (result == SVGPathReadResult::End)
                break;

            if (result == SVGPathReadResult::Error)
                return false;

            if (!normalizer.consume(cmd, args, repeated))
                return false;
        }

        if (!builder.end())
            return false;

        outProg = std::move(builder.prog);
        return true;
    }

    // ------------------------------------------------------------
    // parsePathProgram()
    // 
    // build a PathProgram from path data, 
    // represented by SVG <path> 'd' attribute.
    // The program is a canonicalized, normalized representation of the path data,
    // so, there are no relative commands, no implicit lineto after moveto, 
    // arcs are in endpoint form, etc.
    // ------------------------------------------------------------

    static inline bool parsePathProgram(const ByteSpan& inSpan, PathProgram& outProg) noexcept
    {
        PathProgramBuilder builder;
        PathSegmentNormalizer normalizer(builder);

        SVGSegmentIterator iter(inSpan);
        PathSegment seg{};

        while (svgPathSegment_read(iter, seg)) {
            normalizer.consume(seg);
        }

        builder.end();
        outProg = std::move(builder.prog);
        return true;
    }
}
