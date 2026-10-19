# Finite translation covers — BIL-UNFOLD-02

Reflecting the polygon instead of its moving point lets the point continue
straight. A copy records an orthogonal matrix from the finite group generated
by reflections parallel to the polygon sides. Across a side, the next matrix
is the old matrix followed by that side reflection. Corresponding endpoints
then differ by one translation. That translation is the chart transition;
the flow vector will not reflect in the cover.

The code enumerates this group exactly in Q(√3), with at most 24 copies.
The number of copies must equal twice the least common multiple of the reduced
angle denominators. Unsupported or uncertified polygons cannot manufacture a
finite cover. Arithmetic and copy limits leave the caller's output unchanged.

All side labels refer to the original polygon. A negative-determinant copy
traverses that boundary in reverse when viewed with the surface orientation.
Glued copies have opposite determinant signs, so their oriented boundaries
are paired in reverse even though corresponding original vertex labels match.
The validator checks both endpoints, inverse pairings, matrix relations,
orthogonality, distinct copies, orientation, and connectedness.

The original billiard, its orientation double and this translation cover are
different objects. The square cover here has four unit-square charts, area 4,
and torus periods 2 and 2i. It is not the one-chart unit-area square torus.
Likewise, the L billiard yields four L charts with area 12, genus two and one
6π cone point. It is not silently replaced by the area-3 three-square surface.
Regular vertex classes are retained as marked points of cone angle 2π and
zero order zero. They are not singularities.

Corner identifications give V. Each paired side gives an edge, and each copy
gives a face. Euler characteristic V − E + F determines genus. Summing the
exact polygon angles at each identified corner gives its cone angle and zero
order; the independent identity Σ zero orders = 2g − 2 checks the result.
The test program reconstructs corner cycles by graph traversal independently
of the production union/find calculation. Hand-derived controls are recorded
in `tests/surface-controls.tsv`; malformed pairings and metadata must fail.

The TSV serializer records the model, all seam translations, cone classes,
genus and exact area. A drawing in one plane is not a global chart or a
topology certificate. Sage/flatsurf is not an application dependency, and no
external Sage run or separate-worker acceptance is claimed by these tests.
