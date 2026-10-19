# Straight-line flow — BIL-FLOW-03

A point moves with one global direction on the translation cover. In each
chart its direction is pulled back through the copy matrix to find the next
polygon boundary. Crossing the paired seam changes chart by a translation;
the global direction stays exactly the same. Folding the result back through
the new copy matrix gives the familiar reflected billiard segment.

The state keeps exact flight parameter, current copy, folded position,
accumulated chart translation and a fixed global direction. Its developed
position is initial position + parameter × direction. Every step checks that
this straight line agrees with the current chart. Rendering and floating
distance estimates cannot alter this state.

The parameter budget is exact. Geometric distance is parameter × direction
length; that length is a display estimate when its square root is outside the
field. A renderer reveals partial segments by distance without feeding a
rounded endpoint back into the flow. The separate crossing budget bounds
work. DISTANCE_LIMIT, CROSSING_LIMIT and ARITHMETIC_LIMIT mean UNKNOWN.

A first return to the same point, chart and direction gives an exact periodic
witness. The search checks inside every segment as well as at boundaries;
it cannot miss a shorter return between seam crossings. Regular marked points
have only one tangent plane, so a return to the same regular vertex class also
counts. A finite run without this witness never implies aperiodicity, density,
minimality or ergodicity.

Polygon vertices and cone singularities have different meanings. The polygon
kernel stops at every vertex. The cover engine consults its verified cone
order. At a regular 2π point it finds the outgoing chart containing the global
direction, then continues. At a positive zero order it stops. An explicit
vertex-departure API also names a starting chart/sector; it never silently
continues the polygon engine through a vertex or chooses a direction through
a cone singularity. The original rectangle UI's corner reversal is unchanged.

Tests compare square/rectangle foldback with independent mirror formulas,
check primitive return parameters in all five fixtures, continue across regular
square vertices, stop at the L surface's genuine cone, and reject a nearly
periodic or irrational-direction finite trace as a periodic certificate.
