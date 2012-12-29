#include "DirPaths.h"

using namespace std;

DirPaths::DirPaths( const char *new_paths )
{
    add(new_paths);
}

DirPaths::DirPaths( const DirPaths& dp )
{
    set(dp);
}

DirPaths& DirPaths::operator =( const DirPaths &dp )
{
    if (this != &dp)
        set(dp);
    return *this;
}

void DirPaths::set( const DirPaths &dp )
{
    paths = dp.paths;
    all_paths = dp.all_paths;
}

void DirPaths::add( const DirPaths &dp )
{
    add(dp.get_all_paths());
}

void DirPaths::add( const char *new_paths )
{
    //don't add null paths
    if (new_paths == NULL) return;
    add(string(new_paths));
}

void DirPaths::add( const string &new_paths )
{
    if (new_paths.empty()) {
        if (paths.empty())
            paths.push_back(new_paths);
        return;
    }

    cerr << "Adding path: " << new_paths << endl;

    if (paths.size() == 1 && paths[0].empty()) {
        // was an "empty path"
        // keep the "" as the first of the paths by making it a "."
        paths[0] = ".";
        paths[0].push_back(DELIMITER);
    }

    int p1 = 0, p2 = new_paths.find(PATH_DELIMITER);
    while (p2 != string::npos) {
        if (p1 != p2) { // ignore empty string paths
            paths.push_back(string(new_paths, p1, p2 - p1));
            if (*(paths.back().rbegin()) != DELIMITER) {
                // put a slash on the end if there isn't one alreadey
                paths.back().push_back(DELIMITER);
            }
        }
        p1 = p2 + 1;
        p2 = new_paths.find(p1, PATH_DELIMITER);
    }
    paths.push_back(string(new_paths, p1));
    if (*(paths.back().rbegin()) != DELIMITER)
        paths.back().push_back(DELIMITER);

    // construct all_paths
    all_paths = "";
    for (int i = 0; i < paths.size() - 1; i++) {
        all_paths += paths[i];
        all_paths.push_back(PATH_DELIMITER);
    }
    all_paths += paths.back();
}

const char* DirPaths::get_path( int n ) const
{
    if (0 <= n && n < paths.size())
        return paths[n].c_str();
    else
        return NULL;
}

// Returns a delimited string containing all paths
const char* DirPaths::get_all_paths() const
{
    return all_paths.c_str();
}

int DirPaths::get_num_paths() const
{
    return paths.size();
}

// Returns the length of the longest path
size_t DirPaths::max_path_len() const
{
    size_t len = 0;
    for (int i = 0; i < paths.size(); i++)
        if (paths[i].length() > len)
            len = paths[i].length();
    return len;
}
