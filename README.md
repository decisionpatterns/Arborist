
## Arborist: Parallelized, Extensible Random Forests


[![License](https://img.shields.io/badge/core-MPL--2-brightgreen.svg)](https://www.mozilla.org/en-US/MPL/2.0/) 
[![R License](http://img.shields.io/badge/R_Bridge-GPL%20%28%3E=%202%29-brightgreen.svg?style=flat)](http://www.gnu.org/licenses/gpl-2.0.html)
[![Python License](http://img.shields.io/badge/Python__Bridge-MIT-brightgreen.svg?style=flat)](https://opensource.org/licenses/MIT
)
[![CRAN](http://www.r-pkg.org/badges/version/Rborist)](https://cran.rstudio.com/web/packages/Rborist/index.html)
[![Downloads](http://cranlogs.r-pkg.org/badges/Rborist?color=brightgreen)](http://www.r-pkg.org/pkg/Rborist)
[![PyPI version](https://badge.fury.io/py/pyborist.svg)](https://pypi.python.org/pypi/pyborist/) 
[![Travis-CI Build Status](https://travis-ci.org/suiji/Arborist.svg?branch=master)](https://travis-ci.org/suiji/Arborist)




The Arborist provides a fast, open-source implementation of Leo Brieman's Random Forest algorithm. The Arborist achieves its speed through efficient C++ code and parallel, distributed tree construction. 

Bindings are available for [R](https://cran.r-project.org/web/packages/Rborist/index.html) and **Python (coming soon)**


### R

The *Arborist* is available on CRAN as the [Rborist](https://cran.r-project.org/web/packages/Rborist/index.html) package. 

Installation of Release Version:

    > install.packages('Rborist')

Installation of Development Version:

    > -tk


#### Notes
- Version 1.0 now on CRAN.

### Python

The *Arborist* will soon be available on PyPI.

### Performance 

Performance metrics will be measured soon using [benchm-ml](https://github.com/szilard/benchm-m). Partial results can be found [here](https://github.com/szilard/benchm-ml/tree/master/z-other-tools)

    
### References

- [Controlling for Monotonicity in Random Forest Regressors (PDF)](http://www.rinfinance.com/agenda/2016/talk/MarkSeligman.pdf), R in Finance, May 2016.
- [GTC 2015 Poster, March 2015 (Video)](http://on-demand.gputechconf.com/gtc/2015/posters/GTC_2015_Machine_Learning___Deep_Learning_03_P5282_WEB.pdf)
- [PyData, August 2015 (PDF)](https://www.youtube.com/watch?v=dRZrYdhNUec)
- [R in Finance 2015](http://www.rinfinance.com/agenda/2015/talk/MarkSeligman.pdf)


### News/Changes
- Several major refactorings have taken place since the initial CRAN release.  These were aimed not only at improving performance but also away from the Arborist's original narrow focus on high observation count and high predictor probability.  Additionally, the groundwork has been laid for more advanced features to appear in upcoming releases, such as support for heterogeneous parallelization and for sparse training sets.
- A "preTrain" feature has been added to cache initital training state.  This will save computation under iterative training schemes, such as are facilitated by the R package Caret.
- Optional vector "regMono" has been introduced to specify monotonic constraints on numeric variables under regression.
- Training now computes and saves full leaf information by default. This facilitates quantile prediction, as well as interaction with other packages, such as "forestFloor", by eliminating the need for customized training.
- Restaging and splitting now parallelize across predictor/node pairs, improving core occupancy.  For an interesting illustration of the limits to the benefits of restaging, see the RangeR preprint by Wright and Ziegler.
- Several performance issues have been resolved, but dynamic load-balancing will likely not receive further attention until version 2.
Correctness errors are being addressed as they are received.

