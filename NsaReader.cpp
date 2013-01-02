#include "NsaReader.h"
#include <cstdio>
#define NSA_ARCHIVE_NAME "arc"

using namespace std;

const string NsaReader::nsa_archive_ext = "nsa";
const string NsaReader::ns2_archive_ext = "ns2";

NsaReader::NsaReader( DirPaths &path, int nsaoffset, const unsigned char *key_table )
        :SarReader( path, key_table )
{
    sar_flag = true;
    nsa_offset = nsaoffset;
    num_of_nsa_archives = num_of_ns2_archives = 0;
}

NsaReader::~NsaReader()
{
}

int NsaReader::open_cstr( const char *nsa_path )
{
    return NsaReader::open(string(nsa_path));
}

int NsaReader::open(const string &nsa_path)
{
    DirPaths paths(nsa_path);
    return processArchives(paths);
}

int NsaReader::processArchives( const DirPaths &path )
{
    int i,j,k,n,nd;
    FILE *fp;
    string archive_name, archive_name2;

    if ( !SarReader::open( "arc.sar" ) ) {
        sar_flag = true;
    }
    else {
        sar_flag = false;
    }

    const DirPaths *nsa_path = &path;

    i = j = -1;
    n = nd = 0;
    while ((i<MAX_EXTRA_ARCHIVE) && (n<archive_path->get_num_paths())) {
        if (j < 0) {
            archive_name = nsa_path->get_path(nd) + NSA_ARCHIVE_NAME + "." + nsa_archive_ext;
            archive_name2 = archive_path->get_path(n) + archive_name;
        } else {
            archive_name2 = NSA_ARCHIVE_NAME + std::to_string(j + 1);
            archive_name = nsa_path->get_path(nd) + archive_name2 + "." + nsa_archive_ext;
            archive_name2 = archive_path->get_path(n) + archive_name;
        }
        fp = std::fopen(archive_name2.c_str(), "rb");
        if (fp != NULL) {
            if (i < 0) {
                archive_info_nsa.file_handle = fp;
                archive_info_nsa.file_name = archive_name2;
                readArchive( &archive_info_nsa, ARCHIVE_TYPE_NSA, nsa_offset );
            } else {
                archive_info2[i].file_handle = fp;
                archive_info2[i].file_name = archive_name2;
                readArchive( &archive_info2[i], ARCHIVE_TYPE_NSA, nsa_offset );
            }
            i++;
            j++;
        } else {
            j = -1;
            nd++;
            if (nd >= nsa_path->get_num_paths()) {
                nd = 0;
                n++;
            }
        }
    }

    k = 0;
    n = nd = 0;
    while ((k<MAX_NS2_ARCHIVE) && (n<archive_path->get_num_paths())) {
        archive_name = nsa_path->get_path(nd) + "00." + ns2_archive_ext;
        archive_name2 = archive_path->get_path(n) + archive_name;
        fp = std::fopen(archive_name2.c_str(), "rb");
        if (fp != NULL) {
            fclose(fp);
            for (j=MAX_NS2_ARCHIVE_NUM; (j>=0) && (k<MAX_NS2_ARCHIVE); j--) {
                archive_name = nsa_path->get_path(nd);
                archive_name.push_back('0' + j / 10);
                archive_name.push_back('0' + j % 10);
                archive_name += "." + ns2_archive_ext;
                archive_name2 = archive_path->get_path(n) + archive_name;
                fp = std::fopen(archive_name2.c_str(), "rb");
                if (fp == NULL) {
                    if (j == 0) break;
                } else {
                    archive_info_ns2[k].file_handle = fp;
                    archive_info_ns2[k].file_name = archive_name2;
                    readArchive( &archive_info_ns2[k], ARCHIVE_TYPE_NS2 );
                    k++;
                }
            }
        }
        nd++;
        if (nd >= nsa_path->get_num_paths()) {
            nd = 0;
            n++;
        }
    }

    if ((i < 0) && (k < 0)) {
        // didn't find any (main) archive files
        cerr << "can't open nsa archive file " << NSA_ARCHIVE_NAME << '.' << nsa_archive_ext;
        cerr << "or ns2 archive file 00." << ns2_archive_ext;
        return -1;
    } else {
        num_of_nsa_archives = i+1;
        num_of_ns2_archives = k;
        return 0;
    }
}

const string NsaReader::getArchiveName() const
{
    return "nsa";
}

const char *NsaReader::getArchiveName_cstr() const
{
    return "nsa";
}

int NsaReader::getNumFiles(){
    int i;
    int total = archive_info.num_of_files; // start with sar files, if any

    total += archive_info_nsa.num_of_files; // add in the arc.nsa files

    for ( i=0 ; i<num_of_nsa_archives-1 ; i++ ) total += archive_info2[i].num_of_files; // add in the arc?.nsa files

    for ( i=0 ; i<num_of_ns2_archives ; i++ ) total += archive_info_ns2[i].num_of_files; // add in the ##.ns2 files

    return total;
}

size_t NsaReader::getFileLengthSub(ArchiveInfo *ai, const string &file_name)
{
    unsigned int i = getIndexFromFile( ai, file_name );

    if ( i == ai->num_of_files ) return 0;

    if ( ai->fi_list[i].original_length != 0 ){
        return ai->fi_list[i].original_length;
    }

    int type = ai->fi_list[i].compression_type;
    if ( type == NO_COMPRESSION )
        type = getRegisteredCompressionType( file_name );
    if ( type == NBZ_COMPRESSION || type == SPB_COMPRESSION ) {
        ai->fi_list[i].original_length = getDecompressedFileLength( type, ai->file_handle, ai->fi_list[i].offset );
    }
    
    return ai->fi_list[i].original_length;
}

size_t NsaReader::getFileLength_cstr( const char *file_name )
{
    size_t ans = NsaReader::getFileLength(string(file_name));
    return ans;
}

size_t NsaReader::getFileLength(const string &file_name)
{
    size_t ret;
    int i;
    
    // direct read
    if ( ( ret = DirectReader::getFileLength( file_name ) ) ) return ret;

    // ns2 read
    for ( i=0 ; i<num_of_ns2_archives ; i++ ){
        if ( (ret = getFileLengthSub( &archive_info_ns2[i], file_name )) ) return ret;
    }
    
    // nsa read
    if ( ( ret = getFileLengthSub( &archive_info_nsa, file_name )) ) return ret;

    // nsa? read
    for ( i=0 ; i<num_of_nsa_archives-1 ; i++ ){
        if ( (ret = getFileLengthSub( &archive_info2[i], file_name )) ) return ret;
    }
    
    // sar read
    if ( sar_flag ) return SarReader::getFileLength( file_name );

    return 0;
}

size_t NsaReader::getFile_cstr( const char *file_name, unsigned char *buffer, int *location )
{
    return NsaReader::getFile(file_name, buffer, location);
}

size_t NsaReader::getFile(const string &file_name, unsigned char *buffer, int *location)
{
    size_t ret;

    // direct read
    if ( ( ret = DirectReader::getFile( file_name, buffer, location ) ) ) return ret;

    // ns2 read
    for ( int i=0 ; i<num_of_ns2_archives ; i++ ){
        if ( (ret = getFileSub( &archive_info_ns2[i], file_name, buffer )) ){
            if ( location ) *location = ARCHIVE_TYPE_NS2;
            return ret;
        }
    }

    // nsa read
    if ( (ret = getFileSub( &archive_info_nsa, file_name, buffer )) ){
        if ( location ) *location = ARCHIVE_TYPE_NSA;
        return ret;
    }

    // nsa? read
    for ( int i=0 ; i<num_of_nsa_archives-1 ; i++ ){
        if ( (ret = getFileSub( &archive_info2[i], file_name, buffer )) ){
            if ( location ) *location = ARCHIVE_TYPE_NSA;
            return ret;
        }
    }

    // sar read
    if ( sar_flag ) return SarReader::getFile( file_name, buffer, location );

    return 0;
}

struct NsaReader::FileInfo NsaReader::getFileByIndex( unsigned int index )
{
    int i;
    
    for ( i=0 ; i<num_of_ns2_archives ; i++ ){
        if ( index < archive_info_ns2[i].num_of_files ) return archive_info_ns2[i].fi_list[index];
        index -= archive_info_ns2[i].num_of_files;
    }
    if ( index < archive_info_nsa.num_of_files ) return archive_info_nsa.fi_list[index];
    index -= archive_info_nsa.num_of_files;

    for ( i=0 ; i<num_of_nsa_archives-1 ; i++ ){
        if ( index < archive_info2[i].num_of_files ) return archive_info2[i].fi_list[index];
        index -= archive_info2[i].num_of_files;
    }
    fprintf( stderr, "NsaReader::getFileByIndex  Index %d is out of range\n", index );

    return archive_info.fi_list[0];
}
