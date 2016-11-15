===============================================
MRCZ Compressed MRC file-format package (C99)
===============================================

Author: Robert A. McLeod

Email: robbmcleod@gmail.com

c-MRCZ is a package designed to supplement the venerable MRC image file format with a highly efficient compressed variant, using the Blosc meta-compressor library 
to shrink files on disk and greatly accelerate file input/output for the era of "Big Data" in electron (and optical) microscopy.

c-MRCZ is currently in alpha. 

c-MRCZ is written in C99 for maximum backwards compatibility.  It is designed to be a minimalist implementation of the specification, such that it may be used as a 
template by other 


Installation
------------

c-MRCZ has the following dependencies:

* A GNU C-99 compatible compiler (Windows support is on the TODO list).
* `CMake` version 2.8 or later.
* `c-blosc` (downloaded automatically by CMake).
* `git` (TO BE removed later).

On a Ubuntu/Debian Linux computer:

    sudo apt-get install cmake

On RHEL/CentOS:

    sudo yum install cmake

(Git should be pre-installed on most Linux distros.)

Then navigate to where you would like to install c-mrcz (such as ~/mrcz), and clone the git repo:

    git clone https://github.com/em-MRCZ/c-mrcz.git
    
    cd c-mrcz
    mkdir build
    cd build
    cmake ..
    make all -j 4

It may be necessary at present to re-run `make` as the cmake script downloads blosc from git (on the issues list TODO).


c-MRCZ is released under the BSD license.

Command-line Tutorial
---------------------

TODO.

    mrcz -i <input_file> -o <output_file> [-c <compressor> -B <blocksize> -l <compression_level> -f <filter_enum> -n <# threads> ]

    -c is one of 'none', 'lz4', 'lz4hc', 'zlib', or 'zstd' (default).
    -B is the size of each compression block in bytes (default: 131072).
    -l is compression level, 0 is uncompressed, 9 is very slow (default: 1). Compression ratio with 'zstd' saturates at about 4.
    -f is the filter, 0 is no filter, 1 is byte-shuffle, 2 is bit-shuffle (default).  
    -n is the number of threads, this is best as the number of virtual cores (default: 4).


Library Usage Examples
----------------------

[TODO]

The return type from `mrcVolume_data( vol )` is a void-pointer so the user is responsible for casting it.  This can be done with a switch-case, or by checking which of the 
pointers in the `mrcVolume` struct is `!= NULL`.  

Feature List
------------

* I/O: MRC and MRCZ
* Compress and bit-shuffle image stacks and volumes with `blosc` meta-compressor


Citations
---------

* A. Cheng et al., "MRC2014: Extensions to the MRC format header for electron cryo-microscopy and tomography", Journal of Structural Biology 192(2): 146-150, November 2015, http://dx.doi.org/10.1016/j.jsb.2015.04.002
* V. Haenel, "Bloscpack: a compressed lightweight serialization format for numerical data", arXiv:1404.6383


