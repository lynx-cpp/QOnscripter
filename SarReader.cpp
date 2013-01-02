#include "SarReader.h"
#define WRITE_LENGTH 4096

using namespace std;

SarReader::SarReader( DirPaths &path, const unsigned char *key_table )
    :DirectReader(path)
{
    root_archive_info = last_archive_info = &archive_info;
    num_of_sar_archives = 0;
}

SarReader::~SarReader()
{
    close();
}

int SarReader::open_cstr( const char *name )
{
    return SarReader::open(string(name));
}

int SarReader::open(const string &name)
{
    ArchiveInfo* info = new ArchiveInfo();

    if ( (info->file_handle = fopen( name, "rb" ) ) == NULL ){
        delete info;
        return -1;
    }

    info->file_name = name;
    
    readArchive( info );

    last_archive_info->next = info;
    last_archive_info = last_archive_info->next;
    num_of_sar_archives++;

    return 0;
}

int SarReader::readArchive( struct ArchiveInfo *ai, int archive_type, int offset )
{
    unsigned int i=0;
    
    /* Read header */
    for (int j=0; j<offset; j++)
        i = readChar( ai->file_handle );
    if ( archive_type == ARCHIVE_TYPE_NS2 ) {
        // new archive type since NScr2.91
        // - header starts with base_offset (byte-swapped), followed by
        //   filename data - doesn't tell how many files!
        // - filenames are surrounded by ""s
        // - new NS2 filename def: "filename", length (4bytes, swapped)
        // - no compression type? really, no compression.
        // - not sure if NS2 uses key_table or not, using default funcs for now
        ai->base_offset = swapLong( readLong( ai->file_handle ) );
        ai->base_offset += offset;

        // need to parse the whole header to see how many files there are
        ai->num_of_files = 0;
        long unsigned int cur_offset = offset + 5;
        // there's an extra byte at the end of the header, not sure what for
        while (cur_offset < ai->base_offset){
            //skip the beginning double-quote
            unsigned char ch = fgetc(ai->file_handle);
            cur_offset++;
            do cur_offset++;
            while ((ch = fgetc(ai->file_handle)) != '"');
            i = readLong( ai->file_handle );
            cur_offset += 4;
            ai->num_of_files++;
        }
        ai->fi_list = new struct FileInfo[ ai->num_of_files ];

        // now go back to the beginning and read the file info
        cur_offset = ai->base_offset;
        fseek( ai->file_handle, 4 + offset, SEEK_SET );
        for ( i=0 ; i<ai->num_of_files ; i++ ){
            unsigned int count = 0;
            //skip the beginning double-quote
            unsigned char ch = fgetc(ai->file_handle);
            //error if _not_ a double-quote
            if (ch != '"') {
                fprintf(stderr, "file does not seem to be a valid NS2 archive\n");
                return -1;
            }
            while((ch = fgetc( ai->file_handle )) != '"'){
                if ( 'a' <= ch && ch <= 'z' ) ch += 'A' - 'a';
                ai->fi_list[i].name.push_back(ch);
            }
            ai->fi_list[i].compression_type = getRegisteredCompressionType(ai->fi_list[i].name);
            ai->fi_list[i].offset = cur_offset;
            ai->fi_list[i].length = swapLong( readLong( ai->file_handle ) );
            ai->fi_list[i].original_length = ai->fi_list[i].length;
            cur_offset += ai->fi_list[i].length;
        }
        //
        // old NSA filename def: filename, ending '\0' byte , compr-type byte,
        // start (4byte), length (4byte))
    } else {
        ai->num_of_files = readShort( ai->file_handle );
        ai->fi_list = new struct FileInfo[ ai->num_of_files ];

        ai->base_offset = readLong( ai->file_handle );
        ai->base_offset += offset;

        for ( i=0 ; i<ai->num_of_files ; i++ ){
            unsigned char ch;
            int count = 0;

            while( (ch = fgetc( ai->file_handle ) ) ){
                if ( 'a' <= ch && ch <= 'z' ) ch += 'A' - 'a';
                ai->fi_list[i].name.push_back(ch);
            }

            if ( archive_type == ARCHIVE_TYPE_NSA )
                ai->fi_list[i].compression_type = readChar( ai->file_handle );
            else if (ai->fi_list[i].name.find(".nbz") != string::npos || ai->fi_list[i].name.find(".NBZ") != string::npos)
                ai->fi_list[i].compression_type = NBZ_COMPRESSION;
            else
                ai->fi_list[i].compression_type = NO_COMPRESSION;
            ai->fi_list[i].offset = readLong( ai->file_handle ) + ai->base_offset;
            ai->fi_list[i].length = readLong( ai->file_handle );

            if ( archive_type == ARCHIVE_TYPE_NSA ){
                ai->fi_list[i].original_length = readLong( ai->file_handle );
            }
            else{
                ai->fi_list[i].original_length = ai->fi_list[i].length;
            }

            /* Registered Plugin check */
            if ( ai->fi_list[i].compression_type == NO_COMPRESSION )
                ai->fi_list[i].compression_type = getRegisteredCompressionType(ai->fi_list[i].name);

            //Mion: delaying checking decompressed file length until
            // file is opened for real: original_length = 0 means
            // it hasn't been checked yet
            // (checking every compressed file in this function caused
            //  a massive slowdown at program start when an archive had
            //  many compressed images...)
            if ( (ai->fi_list[i].compression_type == NBZ_COMPRESSION) ||
                  (ai->fi_list[i].compression_type == SPB_COMPRESSION) ){
                ai->fi_list[i].original_length = 0;
            }
        }
    }
    
    return 0;
}

int SarReader::close()
{
    ArchiveInfo *info = archive_info.next;
    
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        last_archive_info = info;
        info = info->next;
        delete last_archive_info;
    }
    num_of_sar_archives = 0;

    return 0;
}

const string SarReader::getArchiveName() const
{
    return "sar";
}

const char *SarReader::getArchiveName_cstr() const
{
    return "sar";
}

int SarReader::getNumFiles(){
    ArchiveInfo *info = archive_info.next;
    int num = 0;
    
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        num += info->num_of_files;
        info = info->next;
    }
    
    return num;
}

int SarReader::getIndexFromFile_cstr( ArchiveInfo *ai, const char *file_name )
{
    return getIndexFromFile(ai, string(file_name));
}

int SarReader::getIndexFromFile(ArchiveInfo *ai, const string &file_name)
{
    unsigned int i;

    capital_name = file_name;

    for (i = 0; i < capital_name.length(); i++) {
        if ( 'a' <= capital_name[i] && capital_name[i] <= 'z' ) capital_name[i] += 'A' - 'a';
        else if ( capital_name[i] == '/' ) capital_name[i] = '\\';
    }
    for (i = 0 ; i < ai->num_of_files; i++)
        if (capital_name == ai->fi_list[i].name) break;

    return i;
}

size_t SarReader::getFileLength_cstr( const char *file_name )
{
    return SarReader::getFileLength(string(file_name));
}

size_t SarReader::getFileLength(const string &file_name)
{
    ArchiveInfo *info = archive_info.next;
    unsigned int j = 0;
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        j = getIndexFromFile( info, file_name );
        if ( j != info->num_of_files ) break;
        info = info->next;
    }
    if ( !info ) return 0;
    
    if ( info->fi_list[j].original_length != 0 ){
        return info->fi_list[j].original_length;
    }

    int type = info->fi_list[j].compression_type;
    if ( type == NO_COMPRESSION )
        type = getRegisteredCompressionType( file_name );
    if ( type == NBZ_COMPRESSION || type == SPB_COMPRESSION ) {
        info->fi_list[j].original_length = getDecompressedFileLength( type, info->file_handle, info->fi_list[j].offset );
    }

    return info->fi_list[j].original_length;
}

size_t SarReader::getFileSub_cstr( ArchiveInfo *ai, const char *file_name, unsigned char *buf )
{
    return SarReader::getFileSub(ai, string(file_name), buf);
}

size_t SarReader::getFileSub(ArchiveInfo *ai, const string &file_name, unsigned char *buf)
{
    unsigned int i = getIndexFromFile( ai, file_name );
    if ( i == ai->num_of_files ) return 0;

    int type = ai->fi_list[i].compression_type;
    if ( type == NO_COMPRESSION ) type = getRegisteredCompressionType( file_name );

    if      ( type == NBZ_COMPRESSION ){
        return decodeNBZ( ai->file_handle, ai->fi_list[i].offset, buf );
    }
    else if ( type == LZSS_COMPRESSION ){
        return decodeLZSS( ai, i, buf );
    }
    else if ( type == SPB_COMPRESSION ){
        return decodeSPB( ai->file_handle, ai->fi_list[i].offset, buf );
    }

    fseek( ai->file_handle, ai->fi_list[i].offset, SEEK_SET );
    size_t ret = fread( buf, 1, ai->fi_list[i].length, ai->file_handle );
    return ret;
}

size_t SarReader::getFile_cstr( const char *file_name, unsigned char *buf, int *location )
{
    return SarReader::getFile(string(file_name), buf, location);
}

size_t SarReader::getFile(const string &file_name, unsigned char *buf, int *location)
{
    size_t ret;
    if ( ( ret = DirectReader::getFile( file_name, buf, location ) ) ) return ret;

    ArchiveInfo *info = archive_info.next;
    size_t j = 0;
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        if ( (j = getFileSub( info, file_name, buf )) > 0 ) break;
        info = info->next;
    }
    if ( location ) *location = ARCHIVE_TYPE_SAR;
    
    return j;
}

struct SarReader::FileInfo SarReader::getFileByIndex( unsigned int index )
{
    ArchiveInfo *info = archive_info.next;
    for ( int i=0 ; i<num_of_sar_archives ; i++ ){
        if ( index < info->num_of_files ) return info->fi_list[index];
        index -= info->num_of_files;
        info = info->next;
    }
    fprintf( stderr, "SarReader::getFileByIndex  Index %d is out of range\n", index );

    return archive_info.fi_list[index];
}
