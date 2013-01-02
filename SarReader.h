#ifndef __SAR_READER_H__
#define __SAR_READER_H__

#include "DirectReader.h"
#include <string>

class SarReader : public DirectReader
{
public:
    SarReader( DirPaths &path, const unsigned char *key_table=NULL );
    ~SarReader();

    int open(const std::string &name);
    int open_cstr( const char *name=NULL );
    int close();
    const std::string getArchiveName() const;
    const char *getArchiveName_cstr() const;
    int getNumFiles();
    size_t getFileLength(const std::string &file_name);
    size_t getFileLength_cstr( const char *file_name );
    size_t getFile(const std::string &file_name, unsigned char *buf, int *location = NULL);
    size_t getFile_cstr( const char *file_name, unsigned char *buf, int *location=NULL );
    struct FileInfo getFileByIndex( unsigned int index );

protected:
    struct ArchiveInfo archive_info;
    struct ArchiveInfo *root_archive_info, *last_archive_info;
    int num_of_sar_archives;

    int readArchive( ArchiveInfo *ai, int archive_type = ARCHIVE_TYPE_SAR, int offset = 0 );
    int getIndexFromFile(ArchiveInfo *ai, const std::string &file_name);
    int getIndexFromFile_cstr( ArchiveInfo *ai, const char *file_name );
    size_t getFileSub(ArchiveInfo *ai, const std::string &file_name, unsigned char *buf);
    size_t getFileSub_cstr( ArchiveInfo *ai, const char *file_name, unsigned char *buf );
};

#endif // __SAR_READER_H__
