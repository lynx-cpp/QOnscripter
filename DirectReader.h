#ifndef __DIRECT_READER_H__
#define __DIRECT_READER_H__

#include "BaseReader.h"
#include "DirPaths.h"
#include <string>

#define MAX_FILE_NAME_LENGTH 256

class DirectReader : public BaseReader
{
public:
    DirectReader(DirPaths &path);
    ~DirectReader();

    int open( const std::string &name );
    int open_cstr( const char *name=NULL );
    int close();

    const std::string getArchiveName() const;
    const char *getArchiveName_cstr() const;
    int getNumFiles();
    void registerCompressionType(const std::string &ext, int type);
    void registerCompressionType_cstr( const char *ext, int type );

    struct FileInfo getFileByIndex( unsigned int index );
    //file_name parameter is assumed to use SJIS encoding
    size_t getFileLength(const std::string &file_name);
    size_t getFileLength_cstr( const char *file_name );
    size_t getFile(const std::string& file_name, unsigned char *buffer, int *location = NULL);
    size_t getFile_cstr( const char *file_name, unsigned char *buffer, int *location=NULL );

    static void convertFromSJISToEUC( char *buf );
    static void convertFromSJISToUTF8( char *dst_buf, char *src_buf );
    
protected:
    std::string file_full_path;
    std::string capital_name;

    DirPaths *archive_path;
    int  getbit_mask;
    size_t getbit_len, getbit_count;
    unsigned char *read_buf;
    unsigned char *decomp_buffer;
    size_t decomp_buffer_len;
    
    struct RegisteredCompressionType{
        RegisteredCompressionType *next;
        std::string ext;
        int type;
        RegisteredCompressionType(){
            next = NULL;
        }
        RegisteredCompressionType( const char *ext, int type ) {
            init(std::string(ext), type);
        }
        RegisteredCompressionType(const std::string &ext, int type) {
            init(ext, type);
        }
        ~RegisteredCompressionType() { }
        private:
            void init(const std::string &ext, int type) {
                for (int i = 0; i < ext.length(); i++)
                    this->ext.push_back(toupper(ext[i]));
                this->type = type;
                this->next = NULL;
            }
    } root_registered_compression_type, *last_registered_compression_type;

    FILE *fopen(const std::string &path, const std::string &mode);
    FILE *fopen_cstr(const char *path, const char *mode);
    unsigned char readChar( FILE *fp );
    unsigned short readShort( FILE *fp );
    unsigned long readLong( FILE *fp );
    void writeChar( FILE *fp, unsigned char ch );
    void writeShort( FILE *fp, unsigned short ch );
    void writeLong( FILE *fp, unsigned long ch );
    static unsigned short swapShort( unsigned short ch );
    static unsigned long swapLong( unsigned long ch );
    size_t decodeNBZ( FILE *fp, size_t offset, unsigned char *buf );
    int getbit( FILE *fp, int n );
    size_t decodeSPB( FILE *fp, size_t offset, unsigned char *buf );
    size_t decodeLZSS( struct ArchiveInfo *ai, int no, unsigned char *buf );
    int getRegisteredCompressionType(const std::string &file_name);
    int getRegisteredCompressionType_cstr( const char *file_name );
    size_t getDecompressedFileLength( int type, FILE *fp, size_t offset );
    
private:
    //file_name parameter is assumed to use SJIS encoding
    FILE *getFileHandle(const std::string &file_name, int &compression_type, size_t *length);
};

#endif // __DIRECT_READER_H__
