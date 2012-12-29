#ifndef __DIR_PATHS__
#define __DIR_PATHS__

#include <iostream>
#include <string>
#include <vector>

#ifdef WIN32
#define DELIMITER '\\'
#define PATH_DELIMITER ';'
#else
#define DELIMITER '/'
#define PATH_DELIMITER ':'
#endif


class DirPaths
{
public:
    DirPaths( const std::string &new_paths );
    DirPaths( const char *new_paths=NULL );
    DirPaths( const DirPaths &dp );
    DirPaths& operator =( const DirPaths &dp );
    
    void add( const char *new_paths );
    void add( const std::string &new_paths );
    void add( const DirPaths &dp );
    const std::string& get_path( int n ) const;
    const std::string& get_all_paths() const;
    const char *get_path_cstr( int n ) const; //paths numbered from 0 to num_paths-1
    const char *get_all_paths_cstr() const;
    int get_num_paths() const;
    size_t max_path_len() const;

private:
    void set( const DirPaths &dp ); //called by copy cons & =op

    std::vector<std::string> paths;
    std::string all_paths;
};

#endif // __DIR_PATHS__
