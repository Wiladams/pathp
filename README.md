# pathp
Parse SVG path attribute into Intermediate Representation

The 'path' element of SVG has a 'd' attribute, which contains a long string describing the geometry of a path.  This pathp decodes that description, and turns it into a 'path program'.  This is an intermediate representation of the path, which is easier to transform into other forms.

This work has its origin in the svgandme project, which is an SVG renderer.  It is separated out here, because the path program has usage outside the confines of the SVG renderer.

