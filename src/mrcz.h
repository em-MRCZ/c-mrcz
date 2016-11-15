/*********************************************************************
  Compressed MRCZ File-format Command-line Utility 

  Author: Robert A. McLeod <robbmcleod@gmail.com>

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#ifndef MRCZ_H
#define MRCZ_H
#endif

#include <limits.h>
#include <stdlib.h>
#include <complex.h>

#include "blosc.h"
// TODO: CMAKE NOT PASSING IN USE_BLOSC define!!!
//#ifdef USE_BLOSC
//    #include "blosc.h"
//#endif

#ifdef __cplusplus
extern "C" {
#endif

// Version information
#define MRCZ_VERSION_MAJOR    0    // for major interface/format changes 
#define MRCZ_VERSION_MINOR    1   // for minor interface/format changes 
#define MRCZ_VERSION_RELEASE  2    // for tweaks, bug-fixes, or development

#define MRCZ_VERSION_STRING   "0.1.2"  // string version.  Sync with above! 
#define MRCZ_VERSION_REVISION "$Rev$"   // revision version
#define MRCZ_VERSION_DATE     "$Date:: 2016-11-15 #$"    // date version

// CMake includes
#if defined(USING_CMAKE)
    #include "mrcz_config.h"
#endif


// Standard (non-extended) header is the default
// We plan to insert meta-data as a footer in the future.
#define MRC_HEADER_LEN              1024 

// Data Types for MRC -- IMOD standard
// Compressed types are MRC_TYPE + (MRC_COMP_RATIO * COMPRESSOR_XXX)
#define MRC_COMP_RATIO              1000  
#define MRC_INT8                    0
#define MRC_INT16                   1
#define MRC_FLOAT32                 2
#define MRC_COMPLEX64               4
#define MRC_UINT16                  6

// Types for compressors
#define BLOSC_NONE_COMPNAME         "none"
#define BLOSC_BLOSCLZ_COMPNAME      "blosclz"
#define BLOSC_LZ4_COMPNAME          "lz4"
#define BLOSC_LZ4HC_COMPNAME        "lz4hc"
#define BLOSC_SNAPPY_COMPNAME       "snappy"
#define BLOSC_ZLIB_COMPNAME         "zlib"
#define BLOSC_ZSTD_COMPNAME         "zstd"

// These are the defines BLOSC_XXX + 1 so that we can have an unpacked 'none' type
#define BLOSC_COMPRESSOR_NONE       0
#define BLOSC_COMRPRESSOR_BLOSCLZ   1
#define BLOSC_COMPRESSOR_LZ4        2
#define BLOSC_COMPRESSOR_LZ4HC      3
#define BLOSC_COMPRESSOR_SNAPPY     4
#define BLOSC_COMPRESSOR_ZLIB       5
#define BLOSC_COMPRESSOR_ZSTD       6

#define BLOSC_NOSHUFFLE             0
#define BLOSC_SHUFFLE               1
#define BLOSC_BITSHUFFLE            2 

#define BLOSC_DEFAULT_THREADS       4
#define BLOSC_DEFAULT_BLOCKSIZE     131072
#define BLOSC_DEFAULT_COMPRESSOR    BLOSC_COMPRESSOR_ZSTD
#define BLOSC_DEFAULT_FILTER        BLOSC_BITSHUFFLE
#define BLOSC_DEFAULT_CLEVEL        1

/*
mrcHeader::

  Holds all the parameters to be read or written in an MRC file header. 
  
Functions::
  
  mrcHeader* mrcHeader_new()
    return a new mrcHeader initialized to default values.
*/
typedef struct _mrcHeader
{
    // Meta-information not included in the standard header
    char* metaname;
    // Pixelsize isn't directly stored in the header but we do store it for user 
    // convience
    float pixelsize[3]; 
    
    // blosc information
    int32_t blosc_compressor;
    int blosc_threads;       // default of zero lets blosc guess the number of threads
    uint8_t blosc_filter;    // defaults to 2 for BITSHUFFLE
    size_t blosc_blocksize;  // default of zero lets blosc guess the number of threads
    uint8_t blosc_clevel;
    
    // MRC fields
    int32_t mrcType;
    int32_t dimensions[3];
    int32_t nStart[3];
    int32_t mGrid[3];
    float cellLen[3];
    float cellAngle[3];
    int32_t mapColRowSlice[3];
    
    float max;
    float min;
    float mean;
    
    int32_t spaceGroup;
    int32_t extendedHeaderSize;
    
    // MRC2000 fields
    float origin;
    uint8_t endian[2];   // TODO: not implemented yet.
    float std;
    
    float voltage;       // in keV
    float C3;            // aka spherical aberration in CEOS formulation
    float gain;          // counts/primary electron
} mrcHeader;

/*
mrcVolume::

  Encapusulates a MRC file, holding both an mrcHeader *header and pointers to 
  all of the available data types. Typically only one of the pointers should 
  not be NULL at any point in time (although it may be helpful to voliate this
  rule for converting data types.)

Functions::
 
  mrcVolume* mrcVolume_new( mrcHeader *header, void *in_array ) 
    returns an mrcVolume struct with allocated memory space. Either argument 
    may be NULL.
    
  void* mrcVolume_data( mrcVolume *vol )
    returns the valid pointer to the active array, according to header->mrcType.
    
    ** The user must cast the return pointer to the appropriate ctype in order 
    to use the returned pointer as an array. **
    
  int mrcVolume_itemsize( mrcVolume *vol )
    returns the itemsize (in bytes) of the data according to header->mrcType  
  
  mrcVolume_free( mrcVolume *vol ) 
    cleans up all memory allocated to the mrcVolume struct.
*/
typedef struct _mrcVolume
{
    mrcHeader *header;
    
    // 'private' data variables, 
    // these can be iterated through to detect which one is not NULL.
    uint8_t  *_u1;
    int8_t   *_i1;
    uint16_t *_u2;
    int16_t  *_i2;
    float    *_f4;
    // TODO: add the complex uint16 type, will probably want an import?  Or a struct?
    complex  *_c8;
    
} mrcVolume;


/* 
  Public library functions 
*/
mrcHeader*   mrcHeader_new()

mrcVolume*   new_mrcVolume( mrcHeader *header, void *data );
size_t       mrcVolume_itemsize( mrcVolume *self );
void         mrcVolume_free( mrcVolume *self )

int          readMRCZ( FILE *fh, mrcVolume *dest, char *filename );
int          writeMRCZ( FILE *fh, mrcVolume *vol );


/* 
  Private library functions 
  You can call these at your own risk, as the implementation interface may 
  change arbitrarily in the future. 
*/
int _parseStandardHeader( uint8_t *headerBytes, mrcHeader *header, char *filename );
uint8_t* _buildStandardHeader( FILE *fh, mrcHeader *header );
int _loadUncompressedMRC( FILE *fh, mrcVolume *dest );
int _decompressMRCZ( FILE *fh, mrcVolume *dest );
int _compressMRCZ( FILE *fh, mrcVolume *source );
void _print_help();

#ifdef __cplusplus
}
#endif