#ifndef __BASE_READER_H__
#define __BASE_READER_H__

#include <cstdio>
#include <iostream>
#include <string>

#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifdef WIN32
#define DELIMITER '\\'
#else
#define DELIMITER '/'
#endif

#define MAX_ERRBUF_LEN 512

struct BaseReader
{
    enum {
        NO_COMPRESSION   = 0,
        SPB_COMPRESSION  = 1,
        LZSS_COMPRESSION = 2,
        NBZ_COMPRESSION  = 4
    };
    
    enum {
        ARCHIVE_TYPE_NONE  = 0,
        ARCHIVE_TYPE_SAR   = 1,
        ARCHIVE_TYPE_NSA   = 2,
        ARCHIVE_TYPE_NS2   = 3   //new format since NScr2.91, uses ext ".ns2"
    };

    struct FileInfo{
        std::string name;
        int  compression_type;
        size_t offset;
        size_t length;
        size_t original_length;
        FileInfo()
        : compression_type(NO_COMPRESSION),
          offset(0), length(0), original_length(0) {
          }
    };

    struct ArchiveInfo{
        struct ArchiveInfo *next;
        FILE *file_handle;
        int power_resume_number; // currently only for PSP
        std::string file_name;
        struct FileInfo *fi_list;
        unsigned int num_of_files;
        unsigned long base_offset;

        ArchiveInfo()
        : next(NULL), file_handle(NULL),
          fi_list(NULL), num_of_files(0), base_offset(0)
        {}
        ~ArchiveInfo(){
            if (file_handle) fclose( file_handle );
            if (fi_list) delete[] fi_list;
        }
    };

    //static char errbuf[MAX_ERRBUF_LEN]; // for passing back error details

    virtual ~BaseReader(){};
    
    virtual int open_cstr( const char *name=NULL ) = 0;
    virtual int open( const std::string &name ) = 0;
    virtual int close() = 0;
    
    virtual const std::string getArchiveName() const = 0;
    virtual const char *getArchiveName_cstr() const = 0;
    virtual int  getNumFiles() = 0;
    virtual void registerCompressionType(const std::string &ext, int type) = 0;
    virtual void registerCompressionType_cstr( const char *ext, int type ) = 0;

    virtual struct FileInfo getFileByIndex( unsigned int index ) = 0;
    //file_name parameter is assumed to use SJIS encoding
    virtual size_t getFileLength(const std::string &file_name) = 0;
    virtual size_t getFileLength_cstr( const char *file_name ) = 0;
    virtual size_t getFile(const std::string &file_name, unsigned char *buffer, int *location = NULL) = 0;
    virtual size_t getFile_cstr( const char *file_name, unsigned char *buffer, int *location=NULL ) = 0;
};

#endif // __BASE_READER_H__
