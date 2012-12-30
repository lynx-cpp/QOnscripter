#ifndef __NSA_READER_H__
#define __NSA_READER_H__

#include "SarReader.h"
#define MAX_EXTRA_ARCHIVE 9
#define MAX_NS2_ARCHIVE_NUM 99
#define MAX_NS2_ARCHIVE 100

class NsaReader : public SarReader
{
public:
    NsaReader( DirPaths &path, int nsaoffset = 0, const unsigned char *key_table=NULL );
    ~NsaReader();

    int open(const std::string &nsa_path);
    int open_cstr( const char *nsa_path=NULL );
    int processArchives( const DirPaths &path );
    const std::string getArchiveName() const;
    const char *getArchiveName_cstr() const;
    int getNumFiles();
    
    size_t getFileLength(const std::string &file_name);
    size_t getFileLength_cstr( const char *file_name );
    size_t getFile(const std::string &file_name, unsigned char *buf, int *location = NULL);
    size_t getFile_cstr( const char *file_name, unsigned char *buf, int *location=NULL );
    struct FileInfo getFileByIndex( unsigned int index );

    int openForConvert( const char *nsa_name, int archive_type=ARCHIVE_TYPE_NSA, int nsaoffset = 0 );
    ArchiveInfo* openForCreate( const char *nsa_name, int archive_type=ARCHIVE_TYPE_NSA, int nsaoffset = 0 );
    int writeHeader( FILE *fp, int archive_type=ARCHIVE_TYPE_NSA, int nsaoffset = 0 );
    size_t putFile( FILE *fp, int no, size_t offset, size_t length, size_t original_length, int compression_type, bool modified_flag, unsigned char *buffer );
private:
    bool sar_flag;
    int nsa_offset;
    int num_of_nsa_archives;
    int num_of_ns2_archives;
    const char *nsa_archive_ext;
    const char *ns2_archive_ext;
    struct ArchiveInfo archive_info_nsa; // for the arc.nsa file
    struct ArchiveInfo archive_info2[MAX_EXTRA_ARCHIVE]; // for the arc1.nsa, arc2.nsa files
    struct ArchiveInfo archive_info_ns2[MAX_NS2_ARCHIVE]; // for the ##.ns2 files

    size_t getFileLengthSub(ArchiveInfo *ai, const std::string &file_name);
    size_t getFileLengthSub_cstr( ArchiveInfo *ai, const char *file_name );
};

#endif // __NSA_READER_H__
