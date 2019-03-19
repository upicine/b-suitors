### b-suitors
Solution is based on [*Efficient approximation algorithms for weighted b-Matching* (Khan, Arif, et al, 2016)](https://www.cs.purdue.edu/homes/apothen/Papers/bMatching-SISC-2016.pdf).

**Graph:** [Pennsylvania road network](http://snap.stanford.edu/data/roadNet-PA.html) 

**CPU:** AMD Opteron(tm) Processor 6386 SE (2.8 GHz, 16 cores)

**Results:**

| Number of threads | Time (in seconds) | Speedup     |
| ----------------- | ----------------- | ----------- |
| 1                 | 30,595           | 1           |
| 2                 | 26,662           | 1,14 |
| 3                 | 24,352           | 1,25 |
| 4                 | 22,781           | 1,34 |
| 5                 | 21,238           | 1,44 |
| 6                 | 19,501           | 1,56 |
| 7                 | 18,898           | 1,61 |
| 8                 | 17,798           | 1,71 |

<sup>Sorry for not the cleanest code :(</sup>