Code repository containing an implementation of One-Hot Encodings.

## Installation

1. Install the emp-toolkit/emp-ot repository by following the instructions in https://github.com/emp-toolkit/emp-ot
2. Create a folder named `build` and cd into it.
3. Run `cmake ..`
4. Run `make`
5. Cd back into base directory

## Benchmarks
1. Cd into the folder named `scripts`
2. Run any of the `.sh`files as follows. In one terminal enter the following
	```./bench_ohe_batched.sh 1 11111 127.0.0.1 iknp ohe 256 1 10```
3. In another terminal, enter the following
	```./bench_ohe_batched.sh 2 11111 127.0.0.1 iknp ohe 256 1 10```
4. For instructions on running other scripts, use the argument `--help`. For example
	```./bench_ohe_batched.sh --help```