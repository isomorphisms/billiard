# Periods and affine deformations — BIL-MODULI-05

Following a closed edge path on the surface develops it into a displacement
in the plane. That displacement is its period. A path between two cone points
also has a displacement, called a relative period. Both come from the actual
gluing; a genus label or an attractive deformed drawing cannot replace them.

The ledger orients each paired edge by its original polygon label. It chooses
a spanning tree in the vertex graph, then a disjoint spanning tree in the dual
face graph. The remaining 2g edges give absolute homology loops: go from the
base vertex through the first tree, traverse the selected edge, and return
through the tree. Paths from the first positive-order cone to the other cones
give the relative periods. The ledger has 2g + s − 1 periods when s>0 cone
points exist, and two absolute periods for a torus without positive zeros.
Regular marked vertices of the cover do not silently become extra relative
coordinates. Each period retains its full integer edge-chain witness.

Tests independently sum those chains and their incidence boundaries. They
check the five canonical covers and an additional genus-three staircase with
two order-2 zeros, which exercises a nontrivial relative period. This is a
chosen local period convention, not a claimed global coordinate system on
moduli space.

The affine layer applies the same 2×2 matrix to every vertex, seam translation
and period. Its rotation, horizontal shear and diagonal Teichmüller-flow
generators use IEEE-754 binary64 arithmetic. The recorded relative gate is
10⁻¹²; nonfinite, orientation-reversing, singular and poorly conditioned maps
are rejected. GL⁺ scales area by its positive determinant. The approximately
determinant-one tag describes a numerical SL(2,ℝ) sample at that precision,
not an exact floating-point theorem. Shears generally change Euclidean length.

After each action the code checks both endpoints of every seam and compares
determinant-scaled area with a fresh polygon-area sum. Tests reconstruct the
transformed periods directly from the transformed polygon edge chains.
Invertibility preserves the combinatorial surface, genus and positive zero
orders; these invariants do not certify an orbit closure.

Only genus one receives a complex torus ratio τ from a primitive lattice
basis. Its determinant must match the exact source area. For an axis-aligned
rectangle, an explicit unimodular change of basis selects (2w,0), (0,2h), so
the undeformed ratio is i·h/w. A horizontal shear adds s·h/w to its real part.
The diagonal action diag(eᵗ,e⁻ᵗ) changes its imaginary part by e⁻²ᵗ. Other
tori retain an oriented cellular lattice basis. Genus two and higher expose
their period ledgers and no single global τ.
