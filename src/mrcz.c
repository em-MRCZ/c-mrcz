/*********************************************************************
  Compressed MRCZ File-format Command-line Utility 

  Author: Robert A. McLeod <robbmcleod@gmail.com>

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

// General library includes
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>

// CMake includes
#if defined(USING_CMAKE)
    #include "mrcz_config.h"
#endif 

// Operating system specific defines
#if defined(_WIN32) && !defined(__MINGW32__)
  #include <windows.h>
  #include <malloc.h>

  /* stdint.h only available in VS2010 (VC++ 16.0) and newer */
  #if defined(_MSC_VER) && _MSC_VER < 1600
    #include "win32/stdint-windows.h"
  #else
    #include <stdint.h>
  #endif

  #include <process.h>
  #define getpid _getpid
  
  // Consider adding wingetopt.h: http://note.sonots.com/Comp/CompLang/cpp/getopt.html
#else /* POSIX or MINGW32 */
  #include <stdint.h>
  #include <unistd.h>
  #include <inttypes.h>
#endif  /* _WIN32 */

#if defined(_WIN32) && !defined(__GNUC__)
  #include "win32/pthread.h"
  #include "win32/pthread.c"
#else
  #include <pthread.h>
#endif

// MRCZ Module includes
#include "mrcz.h"

mrcHeader* mrcHeader_new()
{
    mrcHeader *self = malloc( sizeof( *self ) );

    // Set default values
    self->blosc_threads = BLOSC_DEFAULT_THREADS;
    self->blosc_blocksize = BLOSC_DEFAULT_BLOCKSIZE;
    self->blosc_filter = BLOSC_DEFAULT_FILTER;
    self->blosc_clevel = BLOSC_DEFAULT_CLEVEL;
    self->blosc_compressor = BLOSC_DEFAULT_COMPRESSOR;
    
    return self;
}

mrcVolume* mrcVolume_new( mrcHeader *header, void *in_array )
{
    mrcVolume *self = (mrcVolume*)malloc( sizeof(*self) );
    if( header == NULL )
    {
        self->header = mrcHeader_new();
    }
    else
    { 
        self->header = header; 
    }
    
     // 'Private' data variables
    if( in_array != NULL )
    {
        switch( header->mrcType )
        {
        case MRC_INT8:
            self->_u1 = (int8_t*)in_array;
            break;
        case MRC_INT16:
            self->_i2 = (int16_t*)in_array;
            break;
        case MRC_FLOAT32:
            self->_f4 = (float*)in_array;
            break;
        case MRC_COMPLEX64:
            self->_c8 = (complex*)in_array;
            break;
        case MRC_UINT16:
            self->_u2 = (uint16_t*)in_array;
            break;
        }
    }
    return self;
}


void* mrcVolume_data( mrcVolume *self )
{
    switch( self->header->mrcType )
    {
        case MRC_INT8:
            return (void *)self->_u1;
        case MRC_INT16:
            return (void *)self->_i2;
            
        case MRC_FLOAT32:
            return (void *)self->_f4;
            
        case MRC_COMPLEX64:
            return (void *)self->_c8;
    
        case MRC_UINT16:
            return (void *)self->_u2;
    }
}

size_t mrcVolume_itemsize( mrcVolume *self )
{   // Can't we do this with a macro?
    switch( self->header->mrcType )
    {
        case MRC_INT8:
            return 1;
        case MRC_INT16:
            return 2;
        case MRC_FLOAT32:
            return 4;
        case MRC_COMPLEX64:
            return 8;
        case MRC_UINT16:
            return 2;
    }    
}

void mrcVolume_free( mrcVolume *self )
{
    free( self->header );
    free( self->_u1 );
    free( self->_u2 );
    free( self->_i1 );
    free( self->_i1 );
    free( self->_f4 );
    free( self->_c8 );
    free( self );
}

int _parseStandardHeader( uint8_t *headerBytes, mrcHeader* header, char *metaname )
{
    // Start 
    header->metaname = metaname;

    memcpy( &header->dimensions, &headerBytes[0], sizeof(header->dimensions) );
    memcpy( &header->mrcType, &headerBytes[12], sizeof(header->mrcType) );
    
    if( header->mrcType >= MRC_COMP_RATIO )
    {   // Check if the mrc type/mode is >= 1000, which indicates a compressed type
        header->blosc_compressor = header->mrcType / MRC_COMP_RATIO;
        header->mrcType = header->mrcType % MRC_COMP_RATIO;
    }

    memcpy( &header->nStart, &headerBytes[16], sizeof(header->nStart) );
    memcpy( &header->mGrid, &headerBytes[28], sizeof(header->mGrid) );
    memcpy( &header->cellLen, &headerBytes[40], sizeof(header->cellLen) );
    memcpy( &header->cellAngle, &headerBytes[52], sizeof(header->cellAngle) );
    memcpy( &header->mapColRowSlice, &headerBytes[64], sizeof(header->mapColRowSlice) );

    memcpy( &header->min, &headerBytes[76], sizeof(header->min) );
    memcpy( &header->max, &headerBytes[80], sizeof(header->max) );
    memcpy( &header->mean, &headerBytes[84], sizeof(header->mean) );

    memcpy( &header->spaceGroup, &headerBytes[88], sizeof(header->spaceGroup) );
    memcpy( &header->extendedHeaderSize, &headerBytes[92], sizeof(header->extendedHeaderSize) );

    // MRC2000 fields
    memcpy( &header->origin, &headerBytes[196], sizeof(header->origin) );
    memcpy( &header->endian, &headerBytes[212], sizeof(header->endian) );
    memcpy( &header->std, &headerBytes[216], sizeof(header->std) );

    // CMake defines NDEBUG for _no_ debugging
#ifndef NDEBUG 
    printf( "Metadata name: %s\n", metaname );
    printf( "Dimensions: %i %i %i\n", header->dimensions[0], header->dimensions[1], header->dimensions[2] );
    printf( "MRC type: %i, compressor type: %i\n", header->mrcType, header->blosc_compressor );
    printf( "Cell length: %f, %f, %f\n", header->cellLen[0], header->cellLen[1], header->cellLen[2] );
    printf( "Min: %f, Max: %f, Mean: %f\n", header->min, header->max, header->mean );
    printf( "Extended header size in bytes: %i\n", header->extendedHeaderSize );
#endif

    return 0;
}

uint8_t* _buildStandardHeader( FILE *fh, mrcHeader *header )
{
    static uint8_t headerBytes[MRC_HEADER_LEN];
    int32_t mrcMetaType = header->mrcType + MRC_COMP_RATIO*header->blosc_compressor;
    
    memcpy( &headerBytes[0], &header->dimensions, sizeof(header->dimensions) );
    memcpy( &headerBytes[12], &mrcMetaType, sizeof(mrcMetaType) );

    memcpy( &headerBytes[16], &header->nStart, sizeof(header->nStart) );
    memcpy( &headerBytes[28], &header->mGrid, sizeof(header->mGrid) );
    memcpy( &headerBytes[40], &header->cellLen, sizeof(header->cellLen) );
    memcpy( &headerBytes[52], &header->cellAngle, sizeof(header->cellAngle) );
    memcpy( &headerBytes[64], &header->mapColRowSlice, sizeof(header->mapColRowSlice) );

    memcpy( &headerBytes[76], &header->min, sizeof(header->min) );
    memcpy( &headerBytes[80], &header->max, sizeof(header->max) );
    memcpy( &headerBytes[84], &header->mean, sizeof(header->mean) );

    memcpy( &headerBytes[88], &header->spaceGroup, sizeof(header->spaceGroup) );
    memcpy( &headerBytes[92], &header->extendedHeaderSize, sizeof(header->extendedHeaderSize) );

    // MRC2000 fields
    memcpy( &headerBytes[196], &header->origin, sizeof(header->origin) );
    memcpy( &headerBytes[212], &header->endian, sizeof(header->endian) );
    memcpy( &headerBytes[216], &header->std, sizeof(header->std) );

    // MRCZ2016 fields
    memcpy( &headerBytes[132], &header->voltage, sizeof(header->voltage) );
    memcpy( &headerBytes[136], &header-C3, sizeof(header->C3) );    
    memcpy( &headerBytes[140], &header->gain, sizeof(header->gain) );
    return headerBytes;
}

#define MRC_INT8          0
#define MRC_INT16         1
#define MRC_FLOAT32       2
#define MRC_COMPLEX64     4
#define MRC_UINT16        6
int _loadUncompressedMRC( FILE *fh, mrcVolume *dest )
{
    // mrcVolume *dest must be allocated and have a valid header.
    // fh should point to the position in the file where the data starts.
    size_t dx = dest->header->dimensions[0];
    size_t dy = dest->header->dimensions[1];
    size_t dz = dest->header->dimensions[2];
    size_t dsize = dx*dy*dz;
    int fread_ret = 0;


    switch( dest->header->mrcType )
    {  // Initialize the data union in mrcVolume dest
        
        case MRC_INT8:
            dest->_i1 = malloc( dsize * sizeof(int8_t) );
            fread_ret = fread( dest->_i1, sizeof(int8_t), dsize, fh );
            break;
        case MRC_INT16:
            dest->_i2 = malloc( dsize * sizeof(int16_t) );
            fread_ret = fread( dest->_i2, sizeof(int16_t), dsize, fh );
            break;
        case MRC_FLOAT32:
            dest->_f4 = malloc( dsize * sizeof(float) );
#ifndef NDEBUG
            printf( "_loadUncompressedMRC: Trying to read %lu floats corresponding to %lu bytes\n", dsize, dsize*sizeof(float) );
#endif
            fread_ret = fread( dest->_f4, sizeof(float), dsize, fh );
            break;
            
        case MRC_COMPLEX64:
            dest->_c8 = malloc( dsize * sizeof(complex) );
            fread_ret = fread( dest->_c8, sizeof(complex), dsize, fh );
            break;
        case MRC_UINT16:
            dest->_u2 = malloc( dsize * sizeof(uint16_t) );
            fread_ret = fread( dest->_u2, sizeof(uint16_t), dsize, fh );
            break;
    }
    
#ifndef NDEBUG
    printf( "_loadUncompressedMRC: read %i elements.\n", fread_ret );           
    printf( "_loadUncompressedMRC: location of dest: %p\n", dest );
    printf( "_loadUncompressedMRC: sizeof *dest %lu\n", sizeof(*dest) );
#endif

    return fread_ret;
}

int _decompressMRCZ( FILE *fh, mrcVolume *dest )
{
    // fh must point to the start of the first blsoc1 (32-byte) header
    int blosc_ret;
    size_t dx = dest->header->dimensions[0]; 
    size_t dy = dest->header->dimensions[1];                                
    size_t dz = dest->header->dimensions[2];
    size_t dsize = dx * dy * dz;
    size_t tell_pos;
    size_t itemsize;
    uint8_t *bytesRepr;
    uint8_t *bloscRepr;

    int32_t blosc_header[4];

    if( dest->header->blosc_threads <= 0 )
    {   // We should not get here if we used the mrcHeader_new factory, but a 
        // user might.
        printf( "Warning: blosc_threads set to %d, defaulting to %d.\n", 
               dest->header->blosc_threads, BLOSC_DEFAULT_THREADS );
        dest->header->blosc_threads = BLOSC_DEFAULT_THREADS;
    }
    blosc_set_nthreads( dest->header->blosc_threads );

    switch( dest->header->mrcType )
    {  // Initialize the data union in mrcVolume dest
        
        case MRC_INT8:
            dest->_i1 = malloc( dsize * sizeof(int8_t) );
            itemsize = 1;
            bytesRepr = (uint8_t*)dest->_i1;
            break;
        case MRC_INT16:
            dest->_i2 = malloc( dsize * sizeof(int16_t) );
            itemsize = 2;
            bytesRepr = (uint8_t*)dest->_i2;
            break;
        case MRC_FLOAT32:
            dest->_f4 = malloc( dsize * sizeof(float) );
            itemsize = 4;
            bytesRepr = (uint8_t*)dest->_f4;
            break;
        case MRC_COMPLEX64:
            dest->_c8 = malloc( dsize * sizeof(complex) );
            itemsize = 8;
            bytesRepr = (uint8_t*)dest->_c8;    
            break;
        case MRC_UINT16:
            dest->_u2 = malloc( dsize * sizeof(uint16_t) );
            itemsize = 2;
            bytesRepr = (uint8_t*)dest->_u2;  
            break;
    }

    blosc_init();
    // Iterate through each z-axis slice as a chunk and decompress 
    // each one.
    for( int k = 0; k < dz; k++ )
    {
        // Blosc header format:
        // https://github.com/Blosc/c-blosc/blob/master/README_HEADER.rst
        tell_pos = ftell( fh );
        fread( blosc_header, sizeof(blosc_header), 1, fh ); 
        fseek( fh, tell_pos, SEEK_SET );

#ifndef NDEBUG
        printf( "_decompressMRCZ: blosc_header: flags: %d, nbytes: %d, blocksize: %d, cbytes: %d\n", blosc_header[0], blosc_header[1], blosc_header[2], blosc_header[3]);
#endif
        bloscRepr = malloc( blosc_header[3] );
        fread( bloscRepr, sizeof(uint8_t), blosc_header[3], fh );
        blosc_ret = blosc_decompress_ctx( (void *)bloscRepr, 
                                         (void *)&bytesRepr[itemsize*k*dx*dy], 
                                         itemsize*dx*dy, dest->header->blosc_threads );
                                         

    }
    blosc_destroy();
    return blosc_ret;
}

int _compressMRCZ( FILE *fh, mrcVolume *source )
{
    int blosc_ret, fwrite_ret;
    mrcHeader *header = source->header;
    size_t dx = header->dimensions[0]; 
    size_t dy = header->dimensions[1];                                
    size_t dz = header->dimensions[2];
    size_t itemsize = mrcVolume_itemsize(source);
    const char *compressor_str;

    int32_t blosc_cbytes = itemsize*dx*dy; // Maximum size of slice in compressed bytes
    
    // Do we have to malloc this as the size of an uncompressed slice?
    uint8_t *bloscRepr = malloc( itemsize*dx*dy ); 
    uint8_t *bytesRepr = (uint8_t*)mrcVolume_data(source);

    // Can this switch be macroed?
    switch(source->header->blosc_compressor)
    {
        case(BLOSC_COMRPRESSOR_BLOSCLZ):
            compressor_str = (const char*)BLOSC_BLOSCLZ_COMPNAME;
            break;
        case(BLOSC_COMPRESSOR_LZ4):
            compressor_str = (const char*)BLOSC_LZ4_COMPNAME;
            break;
        case(BLOSC_COMPRESSOR_LZ4HC):
            compressor_str = (const char*)BLOSC_LZ4HC_COMPNAME;
            break;
        case(BLOSC_COMPRESSOR_SNAPPY):
            compressor_str = (const char*)BLOSC_SNAPPY_COMPNAME;
            break;
        case(BLOSC_COMPRESSOR_ZLIB):
            compressor_str = (const char*)BLOSC_ZLIB_COMPNAME;
            break;
        case(BLOSC_COMPRESSOR_ZSTD):
            compressor_str = (const char*)BLOSC_ZSTD_COMPNAME;
            break;
    }
    
#ifndef NDEBUG
    printf( "_compressMRCZ: compressor_str: %s, clevel: %d, filter: %d, blocksize: %lu, threads: %d\n", 
           compressor_str, header->blosc_clevel, header->blosc_filter, header->blosc_blocksize, header->blosc_threads);
#endif

    blosc_init();
    for( int k = 0; k < dz; k++ )
    {
        blosc_ret = blosc_compress_ctx( header->blosc_clevel, 
                                        header->blosc_filter, 
                                        itemsize, 
                                        itemsize*dx*dy, 
                                        (void*) &bytesRepr[itemsize*k*dx*dy], 
                                        bloscRepr, 
                                        blosc_cbytes, 
                                        compressor_str, 
                                        header->blosc_blocksize, 
                                        header->blosc_threads );
        if( blosc_ret == -1 ) 
        { 
            printf( "Error: " );
            return blosc_ret; 
        }

        fwrite_ret = fwrite( bloscRepr, sizeof(uint8_t), blosc_ret, fh);
        if( fwrite_ret <= 0 )
        {
            printf( "Error: _compressMRCZ wrote %d bytes", fwrite_ret );
        }
#ifndef NDEBUG        
        printf( "_compressMRCZ: from %lu to %d bytes, and write: %d bytes\n", itemsize*dx*dy, blosc_ret, fwrite_ret );
#endif
    }
    blosc_destroy();

    return blosc_ret;
}



///////////////////////////////////////////////////////////////////////////////
// MRCZ library public functions
///////////////////////////////////////////////////////////////////////////////
// Consider overloaded readMRCZ( char *filename, mrcVolume *dest ) that opens the file.

int readMRCZ( FILE *fh, mrcVolume *dest, char *name_for_metadata )
{   // Read from a file handle and then write to an address mrcVolume struct, dest.
    // filename is optional and will be saved into the associated dest->header->filename.
    uint8_t headerBytes[MRC_HEADER_LEN];
    int fh_dataStartPos = MRC_HEADER_LEN;
    int fread_ret;
    mrcHeader *header;
    
    header = mrcHeader_new();
    dest->header = header;

    if( ! (fread_ret = fread( (void *)headerBytes, sizeof(uint8_t), MRC_HEADER_LEN, fh ) ) )
    {
        printf( "Error: failed to read 1024-bytes from header of %s, error code: %d\n", name_for_metadata, fread_ret );
        return fread_ret;
    }

    _parseStandardHeader( headerBytes, header, name_for_metadata );

    // Check for presence of extended header
    fh_dataStartPos += header->extendedHeaderSize;
#ifndef NDEBUG
    printf( "DEBUG: seeking to %i in order to read data.\n", fh_dataStartPos );
#endif
    // TODO: read extended header information if desired
    fseek( fh, fh_dataStartPos, SEEK_SET );

    // Branch into compressed or uncompressed implementations
    if( header->blosc_compressor > 0 )
    {   // Compressed data
        _decompressMRCZ( fh, dest );
    }
    else
    {   // Uncompressed data
        fread_ret = _loadUncompressedMRC( fh, dest );
    }
    return fread_ret;
}

int writeMRCZ( FILE *fh, mrcVolume *vol )
{
    // Header
    int fh_dataStartPos = MRC_HEADER_LEN;
    uint8_t *headerBytes;
    void *dataPtr;
    size_t dsize;
    
    headerBytes = _buildStandardHeader( fh, vol->header );
    fh_dataStartPos += vol->header->extendedHeaderSize;

    // TODO: handle writing extended header
    fwrite( headerBytes, sizeof(uint8_t), MRC_HEADER_LEN, fh );

    // Data
#ifndef NDEBUG
    printf( "DEBUG: seeking to %i in order to write data.\n", fh_dataStartPos );
#endif
    // TODO: read extended header information if desired
    fseek( fh, fh_dataStartPos, SEEK_SET );
    if( vol->header->blosc_compressor > 0 )
    {   // Compressed data

        _compressMRCZ( fh, vol );
    }
    else
    {   // Uncompressed data
        dataPtr = mrcVolume_data(vol);
        dsize = vol->header->dimensions[0]*vol->header->dimensions[1]*vol->header->dimensions[2];
        printf( "addressof *dataPtr = %p\n", dataPtr );
        printf( "sizeof *dataPtr = %lu\n", sizeof(*dataPtr) );
        fwrite( dataPtr, mrcVolume_itemsize(vol), dsize, fh );
    }
}

void _print_help()
{
    // IF NO COMMAND ARGS, or -h
    printf( "Usage:  mrcz -i <input_file> -o <output_file> [-c <compressor> -B <blocksize>\n-l <compression_level> -f <filter_enum> -n <# threads> ]\n" );
    printf( "  Takes an input MRC/Z file and transforms it into a compressed/\n  decompressed MRC/Z file.\n" );
    printf( "Options:\n" );
    printf( "    **All arguments apply to the output file only**.\n" );
    printf( "    -c is one of 'none', 'lz4', 'lz4hc', 'zlib', or 'zstd' (default).\n" );
    printf( "    -B is the size of each compression block in bytes (default: 131072).\n" );
    printf( "    -l  is compression level, 0 is uncompressed, 9 is very slow (default: 1). \n        Compression ratio with 'zstd' saturates at about 4.\n" );
    printf( "    -f  is the filter, 0 is no filter, 1 is byte-shuffle, 2 is bit-shuffle (default).\n"  );
    printf( "    -n is the number of threads, typically the number of virtual cores (default: 4).\n" );
}


int main(int argc, char *argv[])
{
    char *inputName, *outputName, *compressor;
    FILE *fh;
    mrcVolume *vol;
    int opt, blocksize=-1, n_threads=-1, filter=-1, clevel=-1;
    
    printf( "Compressed MRC file-format command-line utility, ver.%d.%d.%d\n", 
           MRCZ_VERSION_MAJOR, MRCZ_VERSION_MINOR, MRCZ_VERSION_RELEASE ); 
    
    // PARSE COMMAND LINE ARGUMENTS
    // TODO: getopt not available with MSVC?
    if( argc == 1 )
    {
        _print_help();
        return 0;
    }
    while( (opt = getopt (argc, argv, "i:o:c:B:l:f:n:h") ) != -1)
    {
        switch (opt)
        {
            case 'i':
                //printf("Input file: \"%s\"\n", optarg);
                inputName = optarg;
                break;
            case 'o':
                //printf("Input file: \"%s\"\n", optarg);
                outputName = optarg;
                break;
            case 'c':
                //printf( "compressor: \"%s\"\n", optarg);
                compressor = optarg;
                break;
            case 'B': 
                blocksize = atoi( optarg );
                //printf( "blocksize: \"%d\"\n", blocksize );
                break;
            case 'l':
                clevel = atoi( optarg );
                //printf( "clevel: \"%d\"\n", clevel );
                break;
            case 'f':
                filter = atoi( optarg );
                //printf( "filter: \"%d\"\n", filter );
                break;
            case 'n':
                n_threads = atoi( optarg );
                //printf( "n_threads: \"%d\"\n", n_threads );
                break;
            case 'h':
                _print_help();
                return 0;
        }
    }


    // INPUT READ/DECOMPRESS
    fh = fopen( inputName, "rb" );
    if( fh == NULL )
    {
        printf( "Error: could not open %s to be read.\n", inputName );
        return -1;
    }
    
    vol = mrcVolume_new( NULL, NULL );
    if( ! readMRCZ( fh, vol, inputName ) )
    {   // We have error messages in readMRC
        return -1;
    }
    fclose( fh );


    // Apply command-line options to header
    if( n_threads >  0)
        vol->header->blosc_threads = n_threads;
    if( blocksize >  4096)
        vol->header->blosc_blocksize = blocksize;
    if( filter >= 0 )
        vol->header->blosc_filter = filter;
    if( clevel >= 0 )
        vol->header->blosc_clevel = clevel;
    if( compressor != NULL )
    {
        if( strcmp(compressor, BLOSC_NONE_COMPNAME) == 0 )
            vol->header->blosc_compressor = BLOSC_COMPRESSOR_NONE;
        else if( strcmp(compressor, BLOSC_BLOSCLZ_COMPNAME) == 0 )
            vol->header->blosc_compressor = BLOSC_COMRPRESSOR_BLOSCLZ;
        else if( strcmp(compressor, BLOSC_LZ4_COMPNAME) == 0 )
            vol->header->blosc_compressor = BLOSC_COMPRESSOR_LZ4;
        else if( strcmp(compressor, BLOSC_LZ4HC_COMPNAME) == 0 )
            vol->header->blosc_compressor = BLOSC_COMPRESSOR_LZ4HC;
        else if( strcmp(compressor, BLOSC_SNAPPY_COMPNAME) == 0 )
            vol->header->blosc_compressor = BLOSC_COMPRESSOR_SNAPPY;
        else if( strcmp(compressor, BLOSC_ZLIB_COMPNAME) == 0 )
            vol->header->blosc_compressor = BLOSC_COMPRESSOR_ZLIB;
        else if( strcmp(compressor, BLOSC_ZSTD_COMPNAME) == 0 )
            vol->header->blosc_compressor = BLOSC_COMPRESSOR_ZSTD;
    }
    
    
    // OUTPUT WRITE/COMPRESS
    fh = fopen( outputName, "wb" );
    if( fh == NULL )
    {
        printf( "Error: could not open %s to write.\n", outputName );
        return -1;
    }
    writeMRCZ( fh, vol );
    fclose( fh );


    // Garbage collection (not necessary but this is an example of how to do it)
    mrcVolume_free( vol );
    return 0;
}
