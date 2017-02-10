===============================================
MRCZ meta-compressed file-format package (C99)
===============================================

Author: Robert A. McLeod

Email: robbmcleod@gmail.com

.. contents:: `Table of contents`
    :depth: 2
    :local:

Introduction
============

c-MRCZ is a package designed to supplement the venerable MRC image file format 
with a highly efficient compressed variant, using the Blosc meta-compressor 
library to shrink files on disk and greatly accelerate file input/output for 
the era of "Big Data" in electron (and optical) microscopy. Compared to 
alternative file formats such as HDF5 it is a highly light-weight implemtation 
(~800 lines) and offers high-de/compression rate, and high de/compression ratio 
through the use of the `blosc` meta-compressor library.  `blosc` accelerates 
high-performance compression codecs such as `zstd` and `lz4` by both blocking 
code operations and multi-threaded operation over multiple blocks. In MRCZ 
format each slice along the z-axis is compressed in a seperate chunk, which 
allows any particular slicing operation along the z-axis to be completed without 
decompressing the entire volume. `blosc` also optionally applies a filter to the 
data, each (byte) `SHUFFLE` or `BITSHUFFLE`, where the data is re-arranged in 
its most-significant to least-significant digit. In tests on cryo-TEM data the 
bit-shuffle filter yielded significant improvements in both disk read/write 
times and compression ratio.

c-MRCZ is currently in alpha. 

c-MRCZ is written in C99 for maximum backwards compatibility.  It is designed 
to be a minimalist implementation of the specification, such that it may be 
used as a template by other programmers for their own code. As such it has 
basic factories for two structs, mrcHeader and mrcVolume.  

It also has a dual-application as a command-line utility for converting 
existing MRC files to compressed variants, or equivalently decompressing MRCZ 
files so that legacy software can read the result.  

c-MRCZ and its cousin python-MRCZ are ultra-fast.  Here are some early 
benchmarks that compare compressed to uncompressed performance on a RAID0 hard 
drive (~ 300 MB/s read/write rate)

Stack size: 60 x 3838 x 3710 (3.4 GB) aligned movie .mrcs file.

+---------------+----------------+-----------------+--------------+---------------------+
|Type           |Write time(s)   |Read time(s)     |Size (MB)     |Compression Ratio (%)|
+===============+================+=================+==============+=====================+
|float32        |17.1            |11.5             |3250          |100.0                |
+---------------+----------------+-----------------+--------------+---------------------+
|float32-zstd1  |18.3            |11.4             |2740          |120.0                |
+---------------+----------------+-----------------+--------------+---------------------+
|int8           |2.8             |3.0              |814           |400.0                |
+---------------+----------------+-----------------+--------------+---------------------+
|uint4          |2.6             |6.3              |407           |800.0                |
+---------------+----------------+-----------------+--------------+---------------------+
|int8-zstd1     |1.4             |1.1              |281           |1160.0               |
+---------------+----------------+-----------------+--------------+---------------------+

Downloads
=========

See the downloads page for pre-built binaries:

https://bitbucket.org/emmrcz/c-mrcz/downloads

Compilation
===========

c-MRCZ has the following dependencies:

* A C-99 compatible compiler
* `CMake` version 2.8 or later.
* `c-blosc` (downloaded automatically by CMake).
* `git` (TO BE removed later).

c-MRCZ is released under the BSD license.

Linux
-----

On a Ubuntu/Debian Linux computer::

    sudo apt-get install cmake

On RHEL/CentOS::

    sudo yum install cmake

(Git should be pre-installed on most Linux distros.)

Then navigate to where you would like to install c-mrcz (such as ~/mrcz), and 
clone the git repo::

    git clone https://github.com/em-MRCZ/c-mrcz.git
    
    cd c-mrcz
    mkdir build
    cd build
    cmake ..
    make all -j 4

It may be necessary at present to re-run `make` as the cmake script downloads 
blosc from git (on the issues list TODO).

Windows
-------

For Windows, the need for complex number support requires the use of the C99 
standard, which implies using either MinGW or Visual Studio >= 2012.  If you
are using an earilier version of Visual Studio we assume you have your own 
solution for representing complex numbers so we do not provide one.

Download Visual Studio Community from Microsoft:

    https://www.visualstudio.com/downloads/

Install it, making sure to select to install Visual C++. Then install Cmake and 
Git for Windows:

    https://cmake.org/download/

    https://git-for-windows.github.io
    
Open the command prompt from the Start Menu for either Git Bash or the Command 
Prompt icon associated with Visual Studio.  Navigate to the location you which 
to build the project and enter the following commands::

    git clone https://github.com/em-MRCZ/c-mrcz.git
    
    cd c-mrcz
    mkdir win_build
    cd win_build
    cmake ..
    
Then open the project solution (cmrcz.sln) in Visual Studio and build the
project ALL_BUILD.


Command-line Tutorial
=====================

Basic usage::

    mrcz -i <input_file> -o <output_file> [-c <compressor> -B <blocksize> -l <compression_level> 
      -f <filter_enum> -n <# threads> ]

    -c is one of 'none', 'lz4', 'lz4hc', 'zlib', or 'zstd' (default).

    -B is the size of each compression block in bytes (default: 131072).

    -l is compression level, 0 is uncompressed, 9 is very slow (default: 1). Compression ratio 
      with 'zstd' saturates at about 4.

    -f is the filter, 0 is no filter, 1 is byte-shuffle, 2 is bit-shuffle (default).  

    -n is the number of threads (default: to the number of cores)


Library Usage Examples
======================

[TODO]

The return type from `mrcVolume_data( vol )` is a void-pointer so the user is responsible for casting it.  This can be done with a switch-case, or by checking which of the pointers in the `mrcVolume` struct is `!= NULL`.  

Feature List
============

* I/O: MRC and MRCZ
* Compress and bit-shuffle image stacks and volumes with `blosc` meta-compressor


Citations
=========

1. A. Cheng et al., "MRC2014: Extensions to the MRC format header for electron cryo-microscopy and tomography", Journal of Structural Biology 192(2): 146-150, November 2015, http://dx.doi.org/10.1016/j.jsb.2015.04.002
2. V. Haenel, "Bloscpack: a compressed lightweight serialization format for numerical data", arXiv:1404.6383


