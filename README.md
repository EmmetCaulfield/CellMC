# CellMC

_CellMC_ is an (obsolete, abandoned) XSLT-based cross-platform SBML
model compiler for Intel/AMD x86 and Sony/Toshiba/IBM [Cell/BE processors](https://en.wikipedia.org/wiki/Cell_%28processor%29).

It was written by me (Emmet Caulfield) as part of [my first M.Sc.](http://www.cellmc.org/doc/thesis.pdf) and
abandoned for 15 years. In October of 2023, I imported it into GitHub
from SourceForge as part of consolidating all of my “stuff” in one
place.

# Retrospective (2023)

When _CellMC_ was written in 2008, _XML_ was very much a thing, and
the _Cell/BE_ processor was cool. The _Cell/BE_ code was written
mostly on a _Sony Playstation 3_ running _Yellow Dog Linux_ and tested
on a cluster of _PlayStation 3_s and an [IBM BladeCenter
QS-22](https://en.wikipedia.org/wiki/IBM_BladeCenter#QS22)

Both _Yellow Dog Linux_ and _Cell/BE_ processors reached EOL in 2012. 

If I were implementing it today, I’d do most of it in _Python_ and
would be supporting GPUs rather than Cell/BE.

