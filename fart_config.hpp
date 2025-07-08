#pragma once

#include <string>
#include <vector>

class FartConfig {
public:
    struct Options {
        bool help = false;
        bool quiet = false;
        bool verbose = false;
        bool recursive = false;
        bool count = false;
        bool ignore_case = false;
        bool invert = false;
        bool line_numbers = false;
        bool whole_word = false;
        bool filename_mode = false;
        bool binary = false;
        bool c_style = false;
        bool cvs = false;
        bool svn = false;
        bool git = true;
        bool remove = false;
        bool adapt_case = false;
        bool backup = false;
        bool preview = false;
    };

    struct Statistics {
        int total_files = 0;
        int total_matches = 0;
        
        void reset() {
            total_files = 0;
            total_matches = 0;
        }
    };

    static constexpr const char* VERSION = "v1.99d";
    static constexpr size_t MAX_STRING_SIZE = 8192;
    static constexpr char WILDCARD_SEPARATOR = ',';
    static constexpr const char* WILDCARD_ALL = "*";
    static constexpr const char* TEMP_FILE = "_fart.~";
    static constexpr const char* BACKUP_SUFFIX = ".bak";

    FartConfig() = default;
    
    const Options& getOptions() const { return options_; }
    Options& getOptions() { return options_; }
    
    const Statistics& getStats() const { return stats_; }
    Statistics& getStats() { return stats_; }
    
    const std::string& getWildcard() const { return wildcard_; }
    void setWildcard(const std::string& wildcard) { wildcard_ = wildcard; }
    
    const std::string& getFindString() const { return find_string_; }
    void setFindString(const std::string& find_string) { find_string_ = find_string; }
    
    const std::string& getReplaceString() const { return replace_string_; }
    void setReplaceString(const std::string& replace_string) { replace_string_ = replace_string; }
    
    bool hasWildcard() const { return !wildcard_.empty(); }
    bool hasFindString() const { return !find_string_.empty(); }
    bool hasReplaceString() const { return !replace_string_.empty(); }
    
    bool isGrepMode() const { return hasFindString() && !hasReplaceString(); }
    bool isFartMode() const { return hasFindString() && hasReplaceString(); }
    bool isFindMode() const { return hasWildcard() && !hasFindString(); }

private:
    Options options_;
    Statistics stats_;
    std::string wildcard_;
    std::string find_string_;
    std::string replace_string_;
};