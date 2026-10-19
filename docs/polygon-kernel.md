# Certified polygon collisions — BIL-POLY-01

The kernel follows a point in a polygon until its next boundary intersection.
It reflects the direction at a regular edge and stops at a vertex. Every
intersection, ordering comparison, angle check and reflection uses exact
arithmetic. A close pair of events is never merged using a tolerance.

This implementation is bounded. Coordinates have the form (a + b√3)/d, with
32-bit integer coefficients and a positive denominator. Checked operations
return ARITHMETIC_LIMIT if an intermediate value cannot be represented. No
event or ray state is committed on failure. Conservative limits can occur
even when the eventual reduced answer would fit. This is not arbitrary
precision arithmetic or support for every rational-angle polygon.

Angle certification currently covers sides in multiples of π/12 relative
to the first side. Exact cross products identify parallel directions and
exact dot products select their signs. The interior angle at each vertex
is derived from those headings and checked against the supplied rational
multiple of π. A floating-point angle, a missing tag, or an unsupported
direction cannot certify a polygon. The initial side can have any direction
representable in this field. New angle families require a new exact domain
and certificate implementation; sampled or rounded angles are insufficient.

Geometry is counterclockwise in its coordinate plane. Validation rejects
zero edges, backtracking, nonadjacent crossings or touching, nonpositive
area and incorrect angle claims. Collinear forward edges are allowed with
an interior angle π. Point location uses exact winding and boundary tests.
The kernel receives no pixels, roughness, window, timing or touch state.

The unit square, nonsquare rectangle, right-isosceles triangle, equilateral
triangle with exact √3 height, and three-square right-angled L domain are
controls. A separate integer-determinant oracle checks every first event
and reflected direction on a bounded grid in the rational fixtures. The
equilateral controls use hand-derived exact intersections and reflected
vectors. Repeated rectangle events are compared with mirror folding.
Negative cases include self-crossing, clockwise, zero-area and untagged
input, an unsupported triangle, exact versus nearby corners, nearly
parallel rays, and arithmetic exhaustion. These checks are independent
algorithms within the test program; they are not an external agent review
or a sage-flatsurf acceptance receipt.

`parameter` is exact: end = start + parameter × incoming. Directions are
not normalized, and their squared length is preserved by reflection.
The event's length and cumulative distance are floating display estimates.
They never affect predicates or subsequent mathematical trajectories. A
distance-driven renderer may reveal part of a segment without feeding its
rounded screen position back into the mathematical ray. Each ray also has
an explicit maximum number of events.

`billiard_polygon_rectangle` constructs exact geometry from representable
binary width/height values. The existing rectangle controls, launch
position, angle, wall dragging, pause behavior and corner reversal continue
to use the existing rectangle engine. The new mathematical polygon engine
has a separate vertex-stop policy. Surface flow through a regular marked
point and termination at a genuine cone singularity belong to the later
unfolding/flow jobs; a polygon vertex alone does not establish cone order.

No Android integration or general rational-polygon completion is claimed
by this module. ICK remains the only permitted compiler for its C and test
C. The user authorized execution of this campaign without Flexible Pipes
on 19 October 2026; that exception does not change compiler or evidence rules.
