/*******************************************************************************
 * XEOS - X86 Experimental Operating System
 * 
 * Copyright (c) 2010-2013, Jean-David Gadina - www.xs-labs.com
 * All rights reserved.
 * 
 * XEOS Software License - Version 1.0 - December 21, 2012
 * 
 * Permission is hereby granted, free of charge, to any person or organisation
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to deal in the Software, with or without
 * modification, without restriction, including without limitation the rights
 * to use, execute, display, copy, reproduce, transmit, publish, distribute,
 * modify, merge, prepare derivative works of the Software, and to permit
 * third-parties to whom the Software is furnished to do so, all subject to the
 * following conditions:
 * 
 *      1.  Redistributions of source code, in whole or in part, must retain the
 *          above copyright notice and this entire statement, including the
 *          above license grant, this restriction and the following disclaimer.
 * 
 *      2.  Redistributions in binary form must reproduce the above copyright
 *          notice and this entire statement, including the above license grant,
 *          this restriction and the following disclaimer in the documentation
 *          and/or other materials provided with the distribution, unless the
 *          Software is distributed by the copyright owner as a library.
 *          A "library" means a collection of software functions and/or data
 *          prepared so as to be conveniently linked with application programs
 *          (which use some of those functions and data) to form executables.
 * 
 *      3.  The Software, or any substancial portion of the Software shall not
 *          be combined, included, derived, or linked (statically or
 *          dynamically) with software or libraries licensed under the terms
 *          of any GNU software license, including, but not limited to, the GNU
 *          General Public License (GNU/GPL) or the GNU Lesser General Public
 *          License (GNU/LGPL).
 * 
 *      4.  All advertising materials mentioning features or use of this
 *          software must display an acknowledgement stating that the product
 *          includes software developed by the copyright owner.
 * 
 *      5.  Neither the name of the copyright owner nor the names of its
 *          contributors may be used to endorse or promote products derived from
 *          this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT OWNER AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE AND NON-INFRINGEMENT ARE DISCLAIMED.
 * 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER, CONTRIBUTORS OR ANYONE DISTRIBUTING
 * THE SOFTWARE BE LIABLE FOR ANY CLAIM, DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN ACTION OF CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/* $Id$ */

#include "include/mkinitrd.h"

#define __MKINITRD_CLEANUP          free( ( void * )filepaths );            \
                                    free( ( void * )filenames );            \
                                    free( ( void * )entries );              \
                                    if( files != NULL )                     \
                                    {                                       \
                                        for( i = 0; i < fileCount; i++ )    \
                                        {                                   \
                                            if( files[ i ] != NULL )        \
                                            {                               \
                                                fclose( files[ i ] );       \
                                            }                               \
                                        }                                   \
                                    }                                       \
                                    free( ( void * )files );                \
                                    if( out != NULL )                       \
                                    {                                       \
                                        fclose( out );                      \
                                    }
#define __MKINITRD_ERROR( _s_ )     __MKINITRD_CLEANUP                      \
                                    fprintf( stderr, _s_ );                 \
                                    return EXIT_FAILURE;

int main( int argc, const char * argv[] )
{
    const char  * output;
    const char ** filenames;
    const char ** filepaths;
    FILE        * out;
    FILE       ** files;
    int           i;
    int           j;
    int           fileCount;
    InitRD_Header  header;
    InitRD_Entry * entries;
    InitRD_Entry * entry;
    uint64_t       offset;
    int            verbose;
    char           buf[ 4096 ];
    size_t         size;
    
    verbose     = 0;
    entries     = NULL;
    files       = NULL;
    out         = NULL;
    output      = NULL;
    filepaths   = ( const char ** )calloc( sizeof( const char * ), ( size_t )argc );
    filenames   = NULL;
    fileCount   = 0;
    
    if( filepaths == NULL )
    {
        __MKINITRD_ERROR( "Error: out of memory.\n" );
    }
    
    for( i = 1; i < argc; i++ )
    {
        if( strcmp( argv[ i ], "-h" ) == 0 )
        {
            mkinitrd_help();
            
            return EXIT_SUCCESS;
        }
        else if( strcmp( argv[ i ], "-o" ) == 0 && i != argc - 1 )
        {
            output = argv[ ++i ];
        }
        else if( strcmp( argv[ i ], "-v" ) == 0 )
        {
            verbose = 1;
        }
        else
        {
            filepaths[ fileCount++ ] = argv[ i ];
        }
    }
    
    if( output == NULL )
    {
        __MKINITRD_ERROR( "Error: no output file specified. Please specify an output file with '-o'.\n" );
    }
    
    out = fopen( output, "w" );
    
    if( out == NULL )
    {
        __MKINITRD_ERROR( "Error: cannot open output file for writing.\n" );
    }
    
    if( fileCount == 0 )
    {
        __MKINITRD_ERROR( "Error: no input file specified.\n" );
    }
    
    files     = ( FILE ** )calloc( sizeof( FILE * ), ( size_t )fileCount );
    filenames = ( const char ** )calloc( sizeof( const char * ), ( size_t )fileCount );
    entries   = ( InitRD_Entry * )calloc( sizeof( InitRD_Entry ), ( size_t )fileCount );
    
    if( files == NULL || filenames == NULL || entries == NULL )
    {
        __MKINITRD_ERROR( "Error: out of memory.\n" );
    }
    
    offset = ( uint64_t )sizeof( InitRD_Header ) + ( ( uint64_t )sizeof( InitRD_Entry ) * ( uint64_t )fileCount );
    
    for( i = 0; i < fileCount; i++ )
    {
        filenames[ i ]  = mkinitrd_filename( filepaths[ i ] );
        files[ i ]      = fopen( filepaths[ i ], "r" );
        
        for( j = 0; j < i; j++ )
        {
            if( strcmp( filenames[ i ], filenames[ j ] ) == 0 )
            {
                __MKINITRD_ERROR( "Error: files cannot have the same name.\n" );
            }
        }
        
        if( files[ i ] == NULL )
        {
            __MKINITRD_ERROR( "Error: cannot open input file for writing.\n" );
        }
        
        entry = &( entries[ i ] );
        
        strcpy( entry->filename, filenames[ i ] );
        
        entry->size     = mkinitrd_filesize( files[ i ] );
        entry->offset   = ( uint32_t )offset;
        
        if( offset > UINT32_MAX )
        {
            __MKINITRD_ERROR( "Error: RAM disk is too big.\n" );
        }
        
        if( entry->size == 0 )
        {
            __MKINITRD_ERROR( "Error: file is empty or too big.\n" );
        }
        
        if( verbose == 1 )
        {
            printf
            (
                "File #%i:\n"
                "    Name:      %s\n"
                "    Size:      %010u bytes\n"
                "    Offset:    %010u bytes\n",
                i + 1,
                entry->filename,
                entry->size,
                entry->offset
            );
        }
        
        offset += entry->size;
    }
    
    header.fileCount = ( uint32_t )fileCount;
    
    if( verbose == 1 )
    {
        printf( "Writing RAM disk header:\n    %010lu bytes\n", ( unsigned long )( sizeof( InitRD_Header ) ) );
    }
    
    fwrite( &header, sizeof( InitRD_Header ), 1, out );
    
    if( verbose == 1 )
    {
        printf( "Writing RAM disk entries:\n    %010lu bytes\n", ( unsigned long )( sizeof( InitRD_Entry ) * ( unsigned long )fileCount ) );
    }
    
    fwrite( entries, sizeof( InitRD_Entry ), ( size_t )fileCount, out );
    
    for( i = 0; i < fileCount; i++ )
    {
        memset( buf, 0, sizeof( buf ) );
        
        if( verbose == 1 )
        {
            printf( "Writing data for file #%i:\n", i + 1 );
        }
        
        while( ( size = fread( buf, 1, sizeof( buf ), files[ i ] ) ) )
        {
            if( verbose == 1 )
            {
                printf( "    %010lu bytes\n", ( unsigned long )size );
            }
            
            fwrite( buf, 1, size, out );
        }
    }
    
    __MKINITRD_CLEANUP
    
    return EXIT_SUCCESS;
}
