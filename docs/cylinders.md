# Bounded cylinders and connections — BIL-CYL-04

For one direction, project each chart onto coordinates parallel and
perpendicular to that direction. If every chart vertex height and seam
translation lies in a discrete transverse lattice, cut each chart into strips
between consecutive lattice heights. Each open strip crosses its right edge
into exactly one other strip. The code verifies this finite permutation,
including both endpoints of each boundary interval.

A permutation cycle returns every point of the open strip after the same
positive holonomy. The first cycle return gives the primitive circumference.
The sum of lower and upper widths must agree exactly; areas must partition the
whole surface. Neighboring bands merge across regular closed boundaries with
the same primitive period. A genuine cone point blocks that merge. On an
unmarked torus the result is one certified periodic foliation, rather than
invented saddle connections. This is a finite geometric certificate for the
selected direction, not a density or minimality test.

The coordinate change has determinant equal to the direction's squared norm.
Widths, areas and periods are converted back accordingly. Incidence, matching,
length-cutoff comparisons and area identities use exact Q(√3) arithmetic.
Circumference and height displays alone use square roots and floating values.

Maximal-cylinder boundaries split at true cone points to give straight saddle
connections with no internal singularity. Each record retains its starting
chart, vertex, outgoing cone sector, endpoint class, holonomy and search bound.
Parallel connections with the same holonomy and endpoint are distinct when
they leave different cone sectors. The two boundary views of the same ray
germ are identified. An optional explicitly labelled regular-marked-point
convention is separate from genuine saddle connections.

`billiard_direction_sweep` explores coprime integer pairs in a bounded box,
choosing p>0 or p=0,q>0. Reversed directions and scalar multiples do not count
twice. It records every direction and can write every raw strip/cylinder
certificate. Counts of saddle connections and closed-geodesic families remain
separate. N(L) uses a geometric length cutoff independent of strip, cycle,
direction and output-record limits. A limit or non-discrete transverse set
returns UNKNOWN for the unresolved part.

If all placed vertices and seam translations have integral coordinates, every
saddle-connection/core holonomy is integral. If the direction box reaches L,
all its directions are certified, and no bound truncates work, the sweep can
certify completeness for that finite length cutoff. Otherwise it certifies
only the requested direction set. It never claims an asymptotic constant,
generic orbit closure, density or ergodicity from these counts.

Independent tests restart the straight-flow engine at every recorded cone
sector and require its first singular endpoint to agree. They also trace an
interior point of every strip and require the same primitive return period.
The square torus matches coprime-direction lattice formulas. The L cover has
two horizontal cylinders but one diagonal cylinder; the diagonal first return
has parameter 6 for direction (1,1). An initial three-cylinder expectation was
refuted by independent flow and retained as a counterexample seed. Increasing
length bounds, malformed input, marked-point conventions, irrational probes
and computational limits have separate controls. No external Sage execution
or separate-worker receipt is claimed.
