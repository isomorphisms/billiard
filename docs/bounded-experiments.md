# Bounded orbit samples — BIL-ORBIT-06

The runner builds each exact cover and period ledger, then applies a finite
sequence of shears, diagonal actions and rotations. Every state retains the
full matrix, generator parameter, transformed vertices, seams, periods, genus,
zero orders, area and numerical precision. The default corpus covers the five
fixtures and four successive states per fixture, beginning with identity.

The claim ledger has four classes: fixture-certified mathematics, externally
cited results, finite observations and unknowns. Code admits only the declared
class for each claim. Individual orbit closures, generic density, ergodicity,
orbit dimensions and asymptotic counting constants remain UNKNOWN. Removing a
direction from the finite sample produces NOT OBSERVED, never IMPOSSIBLE.

The external reference is Eskin–Mirzakhani–Mohammadi, *Isolation,
equidistribution, and orbit closures for the SL(2,R) action on moduli space*,
Annals 182 (2015), pp. 673–721, DOI
[10.4007/annals.2015.182.2.7](https://doi.org/10.4007/annals.2015.182.2.7).
The [publisher abstract](https://annals.math.princeton.edu/2015/182-2/p07),
checked 19 October 2026, supports the general orbit-closure/equidistribution
reference. This runner neither reproduces those proofs nor identifies an
individual sample's orbit closure from that citation.

Each sampled direction is transported by the same affine matrix. Cylinder
families and cone-sector connections keep separate counts. Both the source
search cutoff and the transformed geometric cutoff are recorded. A numerical
inverse-matrix bound checks whether the source cutoff can cover compressed
connections; insufficient coverage is labelled. Counts always retain their
finite direction set, source cutoff, strip/cycle caps and uncertainty flags.
Representable binary matrix entries use bounded exact cutoff predicates.
Other binary64 samples have an explicit uncertainty band near the cutoff;
arithmetic exhaustion never silently becomes a rounded certificate.

`make experiments` builds the ICK-native runner and writes `build/bil-orbit.tsv`
plus producer/source/digest records. The build stamps its Git revision and
whether the source is a clean commit or a working tree. A complete dataset
requires `dataset_complete 1` and a successful process exit. The unit test's
all-zero revision is explicitly labelled test_fixture, not real provenance.

CLI controls select fixture, state count, direction box, direction count and
the two length limits. `--summary` omits large raw certificates while retaining
every matrix and per-direction count. Default data retain baseline gluing,
homology chains and strip witnesses. Repeated runs on the same qualified host
are byte-identical; no cross-platform libm bit-identity is claimed. Tables are
finite experiments suitable for comparison, not an asymptotic fit or theorem.
