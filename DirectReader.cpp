#include "DirectReader.h"
#include <bzlib.h>
#if !defined(WIN32) && !defined(MACOS9) && !defined(PSP) && !defined(__OS2__)
#include <dirent.h>
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#define READ_LENGTH 4096
#define WRITE_LENGTH 5000

#define EI 8
#define EJ 4
#define P   1  /* If match length <= P then output one character */
#define N (1 << EI)  /* buffer size */
#define F ((1 << EJ) + P)  /* lookahead buffer size */

using namespace std;

DirectReader::DirectReader(DirPaths &path)
{
    archive_path = &path;

    read_buf = new unsigned char[READ_LENGTH];
    decomp_buffer = new unsigned char[N*2];
    decomp_buffer_len = N*2;

    last_registered_compression_type = &root_registered_compression_type;
    registerCompressionType( "NBZ", NBZ_COMPRESSION );
    registerCompressionType( "SPB", SPB_COMPRESSION );
    registerCompressionType( "JPG", NO_COMPRESSION );
    registerCompressionType( "GIF", NO_COMPRESSION );
}

DirectReader::~DirectReader()
{
    delete[] read_buf;
    delete[] decomp_buffer;
    
    last_registered_compression_type = root_registered_compression_type.next;
    while ( last_registered_compression_type ){
        RegisteredCompressionType *cur = last_registered_compression_type;
        last_registered_compression_type = last_registered_compression_type->next;
        delete cur;
    }
}

FILE *DirectReader::fopen_cstr(const char *path, const char *mode)
{
    return fopen(string(path), string(mode));
}

FILE *DirectReader::fopen(const string &path, const string &mode)
{
    //NOTE: path is likely SJIS, but if called by getFileHandle on
    //      a non-Windows system, it could be UTF-8 or EUC-JP
    FILE *fp = NULL;

    for (int n=0; n<archive_path->get_num_paths(); n++) {
        file_full_path = archive_path->get_path(n) + path;
        fp = ::fopen( file_full_path.c_str(), mode.c_str() );
        if (fp) return fp;
    }

    return fp;
}

unsigned char DirectReader::readChar( FILE *fp )
{
    unsigned char ret = 0;
    fread(&ret, 1, 1, fp);
    return ret;
}

unsigned short DirectReader::readShort( FILE *fp )
{
    unsigned short ret = 0;
    unsigned char buf[2];
    
    if (fread( &buf, 2, 1, fp ) == 1)
        ret = buf[0] << 8 | buf[1];

    return ret;
}

unsigned long DirectReader::readLong( FILE *fp )
{
    unsigned long ret = 0;
    unsigned char buf[4];

    if (fread( &buf, 4, 1, fp ) == 1) {
        ret = buf[0];
        ret = ret << 8 | buf[1];
        ret = ret << 8 | buf[2];
        ret = ret << 8 | buf[3];
    }

    return ret;
}

void DirectReader::writeChar( FILE *fp, unsigned char ch )
{
    if (fwrite( &ch, 1, 1, fp ) != 1)
        fputs("Warning: writeChar failed\n", stderr);
}

void DirectReader::writeShort( FILE *fp, unsigned short ch )
{
    unsigned char buf[2];

    buf[0] = (ch>>8) & 0xff;
    buf[1] = ch & 0xff;
    if (fwrite( &buf, 2, 1, fp ) != 1)
        fputs("Warning: writeShort failed\n", stderr);
}

void DirectReader::writeLong( FILE *fp, unsigned long ch )
{
    unsigned char buf[4];
    
    buf[0] = (unsigned char)((ch>>24) & 0xff);
    buf[1] = (unsigned char)((ch>>16) & 0xff);
    buf[2] = (unsigned char)((ch>>8)  & 0xff);
    buf[3] = (unsigned char)(ch & 0xff);
    if (fwrite( &buf, 4, 1, fp ) != 1)
        fputs("Warning: writeLong failed\n", stderr);
}

unsigned short DirectReader::swapShort( unsigned short ch )
{
    return ((ch & 0xff00) >> 8) | ((ch & 0x00ff) << 8);
}

unsigned long DirectReader::swapLong( unsigned long ch )
{
    return ((ch & 0xff000000) >> 24) | ((ch & 0x00ff0000) >> 8) |
           ((ch & 0x0000ff00) << 8) | ((ch & 0x000000ff) << 24);
}

int DirectReader::open( const string &name )
{
    return 0;
}

int DirectReader::open_cstr( const char *name )
{
    return 0;
}

int DirectReader::close()
{
    return 0;
}

const string DirectReader::getArchiveName() const
{
    return "direct";
}

const char *DirectReader::getArchiveName_cstr() const
{
    return "direct";
}

int DirectReader::getNumFiles()
{
    return 0;
}
    
void DirectReader::registerCompressionType_cstr( const char *ext, int type )
{
    registerCompressionType(string(ext), type);
}

void DirectReader::registerCompressionType(const string &ext, int type)
{
    last_registered_compression_type->next = new RegisteredCompressionType(ext, type);
    last_registered_compression_type = last_registered_compression_type->next;
}
    
int DirectReader::getRegisteredCompressionType_cstr( const char *file_name )
{
    return getRegisteredCompressionType(string(file_name));
}

int DirectReader::getRegisteredCompressionType(const string &file_name)
{
    int ext_buf = file_name.length() - 1;
    while (ext_buf >= 0 && file_name[ext_buf] != '.') ext_buf--;
    ext_buf++;
    
    capital_name = string(file_name, ext_buf);
    for (int i = 0; i < capital_name.size(); i++)
        capital_name[i] = toupper(capital_name[i]);
    
    RegisteredCompressionType *reg = root_registered_compression_type.next;
    while (reg){
        if (capital_name == reg->ext) return reg->type;
        reg = reg->next;
    }

    return NO_COMPRESSION;
}
    
struct DirectReader::FileInfo DirectReader::getFileByIndex( unsigned int index )
{
    DirectReader::FileInfo fi;
    return fi;
}

FILE *DirectReader::getFileHandle(const string &file_name, int &compression_type, size_t *length)
{
    //NOTE: file_name is assumed to use SJIS encoding
    FILE *fp = NULL;

    compression_type = NO_COMPRESSION;
    capital_name = file_name;
    bool has_nonascii = false;
    for (int i = 0; i < capital_name.length(); i++) {
        if ((unsigned char)capital_name[i] >= 0x80)
            has_nonascii = true;
        if ( capital_name[i] == '/' || capital_name[i] == '\\' )
            capital_name[i] = (char)DELIMITER;
    }

    *length = 0;
    if ( ((fp = fopen( capital_name, "rb" )) != NULL) && (file_name.length() >= 3) ){
        compression_type = getRegisteredCompressionType( capital_name);
        if ( compression_type == NBZ_COMPRESSION || compression_type == SPB_COMPRESSION ){
            *length = getDecompressedFileLength( compression_type, fp, 0 );
        }
        else{
            fseek( fp, 0, SEEK_END );
            *length = ftell( fp );
        }
    }

    return fp;
}

size_t DirectReader::getFileLength_cstr( const char *file_name )
{
    return DirectReader::getFileLength(string(file_name));
}

size_t DirectReader::getFileLength(const string &file_name)
{
    int compression_type;
    size_t len;
    FILE *fp = getFileHandle( file_name, compression_type, &len );

    if ( fp ) fclose( fp );
    
    return len;
}

size_t DirectReader::getFile_cstr( const char *file_name, unsigned char *buffer, int *location )
{
    return DirectReader::getFile(string(file_name), buffer, location);
}

size_t DirectReader::getFile(const string &file_name, unsigned char *buffer, int *location)
{
    int compression_type;
    size_t len, c, total = 0;
    FILE *fp = getFileHandle( file_name, compression_type, &len );
    
    if ( fp ){
        if ( compression_type & NBZ_COMPRESSION )
            return decodeNBZ( fp, 0, buffer );
        
        if ( compression_type & SPB_COMPRESSION )
            return decodeSPB( fp, 0, buffer );

        fseek( fp, 0, SEEK_SET );
        total = len;
        while( len > 0 ){
            if ( len > READ_LENGTH ) c = READ_LENGTH;
            else                     c = len;
            len -= c;
            if (fread( buffer, 1, c, fp ) < c) {
                if (ferror( fp ))
                    fprintf(stderr, "Error reading %s\n", file_name.c_str());
            }
            buffer += c;
        }
        fclose( fp );
        if ( location ) *location = ARCHIVE_TYPE_NONE;
    }

    return total;
}

void DirectReader::convertFromSJISToEUC( char *buf )
{
    int i = 0;
    while ( buf[i] ) {
        if ( (unsigned char)buf[i] > 0x80 ) {
            unsigned char c1, c2;
            c1 = buf[i];
            c2 = buf[i+1];

            c1 -= (c1 <= 0x9f) ? 0x71 : 0xb1;
            c1 = c1 * 2 + 1;
            if (c2 > 0x9e) {
                c2 -= 0x7e;
                c1++;
            }
            else if (c2 >= 0x80) {
                c2 -= 0x20;
            }
            else {
                c2 -= 0x1f;
            }

            buf[i]   = c1 | 0x80;
            buf[i+1] = c2 | 0x80;
            i++;
        }
        i++;
    }
}

void DirectReader::convertFromSJISToUTF8( char *dst_buf, char *src_buf )
{
}

size_t DirectReader::decodeNBZ( FILE *fp, size_t offset, unsigned char *buf )
{
    unsigned int original_length, count;
	BZFILE *bfp;
	void *unused;
	int err, len, nunused;

    fseek( fp, offset, SEEK_SET );
    original_length = count = readLong( fp );

	bfp = BZ2_bzReadOpen( &err, fp, 0, 0, NULL, 0 );
	if ( bfp == NULL || err != BZ_OK ) return 0;

	while( err == BZ_OK && count > 0 ){
        if ( count >= READ_LENGTH )
            len = BZ2_bzRead( &err, bfp, buf, READ_LENGTH );
        else
            len = BZ2_bzRead( &err, bfp, buf, count );
        count -= len;
		buf += len;
	}

	BZ2_bzReadGetUnused(&err, bfp, &unused, &nunused );
	BZ2_bzReadClose( &err, bfp );

    return original_length - count;
}

#ifdef TOOLS_BUILD
int a[-1] = {};

size_t DirectReader::encodeNBZ( FILE *fp, size_t length, unsigned char *buf )
{
    unsigned int bytes_in, bytes_out;
	int err;

	BZFILE *bfp = BZ2_bzWriteOpen( &err, fp, 9, 0, 30 );
	if ( bfp == NULL || err != BZ_OK ) return 0;

	while( err == BZ_OK && length > 0 ){
        if ( length >= WRITE_LENGTH ){
            BZ2_bzWrite( &err, bfp, buf, WRITE_LENGTH );
            buf += WRITE_LENGTH;
            length -= WRITE_LENGTH;
        }
        else{
            BZ2_bzWrite( &err, bfp, buf, length );
            break;
        }
	}

	BZ2_bzWriteClose( &err, bfp, 0, &bytes_in, &bytes_out );
    
    return bytes_out;
}

#endif //TOOLS_BUILD

int DirectReader::getbit( FILE *fp, int n )
{
    int i, x = 0;
    static int getbit_buf;
    
    for ( i=0 ; i<n ; i++ ){
        if ( getbit_mask == 0 ){
            if (getbit_len == getbit_count){
                getbit_len = fread(read_buf, 1, READ_LENGTH, fp);
                if (getbit_len == 0) return EOF;
                getbit_count = 0;
            }

            getbit_buf = read_buf[getbit_count++];
            getbit_mask = 128;
        }
        x <<= 1;
        if ( getbit_buf & getbit_mask ) x |= 1;
        getbit_mask >>= 1;
    }
    return x;
}

size_t DirectReader::decodeSPB( FILE *fp, size_t offset, unsigned char *buf )
{
    unsigned int count;
    unsigned char *pbuf, *psbuf;
    size_t i, j, k;
    int c, n, m;

    getbit_mask = 0;
    getbit_len = getbit_count = 0;
    
    fseek( fp, offset, SEEK_SET );
    size_t width  = readShort( fp );
    size_t height = readShort( fp );

    size_t width_pad  = (4 - width * 3 % 4) % 4;

    size_t total_size = (width * 3 + width_pad) * height + 54;

    /* ---------------------------------------- */
    /* Write header */
    for (int i = 0; i < 54; i++)
        buf[i] = 0;
    buf[0] = 'B'; buf[1] = 'M';
    buf[2] = total_size & 0xff;
    buf[3] = (total_size >>  8) & 0xff;
    buf[4] = (total_size >> 16) & 0xff;
    buf[5] = (total_size >> 24) & 0xff;
    buf[10] = 54; // offset to the body
    buf[14] = 40; // header size
    buf[18] = width & 0xff;
    buf[19] = (width >> 8)  & 0xff;
    buf[22] = height & 0xff;
    buf[23] = (height >> 8)  & 0xff;
    buf[26] = 1; // the number of the plane
    buf[28] = 24; // bpp
    buf[34] = total_size - 54; // size of the body

    buf += 54;

    if (decomp_buffer_len < width*height+4){
        if (decomp_buffer) delete[] decomp_buffer;
        decomp_buffer_len = width*height+4;
        decomp_buffer = new unsigned char[decomp_buffer_len];
    }
    
    for ( i=0 ; i<3 ; i++ ){
        count = 0;
        decomp_buffer[count++] = c = getbit( fp, 8 );
        while ( count < (unsigned)(width * height) ){
            n = getbit( fp, 3 );
            if ( n == 0 ){
                decomp_buffer[count++] = c;
                decomp_buffer[count++] = c;
                decomp_buffer[count++] = c;
                decomp_buffer[count++] = c;
                continue;
            }
            else if ( n == 7 ){
                m = getbit( fp, 1 ) + 1;
            }
            else{
                m = n + 2;
            }

            for ( j=0 ; j<4 ; j++ ){
                if ( m == 8 ){
                    c = getbit( fp, 8 );
                }
                else{
                    k = getbit( fp, m );
                    if ( k & 1 ) c += (k>>1) + 1;
                    else         c -= (k>>1);
                }
                decomp_buffer[count++] = c;
            }
        }

        pbuf  = buf + (width * 3 + width_pad)*(height-1) + i;
        psbuf = decomp_buffer;

        for ( j=0 ; j<height ; j++ ){
            if ( j & 1 ){
                for ( k=0 ; k<width ; k++, pbuf -= 3 ) *pbuf = *psbuf++;
                pbuf -= width * 3 + width_pad - 3;
            }
            else{
                for ( k=0 ; k<width ; k++, pbuf += 3 ) *pbuf = *psbuf++;
                pbuf -= width * 3 + width_pad + 3;
            }
        }
    }
    
    return total_size;
}

size_t DirectReader::decodeLZSS( struct ArchiveInfo *ai, int no, unsigned char *buf )
{
    unsigned int count = 0;
    int i, j, k, r, c;

    getbit_mask = 0;
    getbit_len = getbit_count = 0;

    fseek( ai->file_handle, ai->fi_list[no].offset, SEEK_SET );
    for (int i = 0; i < N-F; i++)
        decomp_buffer[i] = 0;
    r = N - F;

    while ( count < ai->fi_list[no].original_length ){
        if ( getbit( ai->file_handle, 1 ) ) {
            if ((c = getbit( ai->file_handle, 8 )) == EOF) break;
            buf[ count++ ] = c;
            decomp_buffer[r++] = c;  r &= (N - 1);
        } else {
            if ((i = getbit( ai->file_handle, EI )) == EOF) break;
            if ((j = getbit( ai->file_handle, EJ )) == EOF) break;
            for (k = 0; k <= j + 1  ; k++) {
                c = decomp_buffer[(i + k) & (N - 1)];
                buf[ count++ ] = c;
                decomp_buffer[r++] = c;  r &= (N - 1);
            }
        }
    }

    return count;
}

size_t DirectReader::getDecompressedFileLength( int type, FILE *fp, size_t offset )
{
    size_t length=0;
    fseek( fp, offset, SEEK_SET );
    
    if ( type == NBZ_COMPRESSION ){
        length = readLong( fp );
    }
    else if ( type == SPB_COMPRESSION ){
        size_t width  = readShort( fp );
        size_t height = readShort( fp );
        size_t width_pad  = (4 - width * 3 % 4) % 4;
            
        length = (width * 3 +width_pad) * height + 54;
    }


    return length;
}
