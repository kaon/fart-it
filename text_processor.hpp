#pragma once

#include <string>
#include <vector>
#include <memory>
#include "fart_config.hpp"

class TextProcessor {
public:
    explicit TextProcessor(const FartConfig& config);
    
    struct FindResult {
        size_t position;
        size_t length;
        std::string replacement;
    };
    
    std::vector<FindResult> findMatches(const std::string& text) const;
    
    std::string processLine(const std::string& line, int& match_count) const;
    
    int countMatches(const std::string& text) const;
    
    bool isWordBoundary(const std::string& text, size_t pos) const;
    
    std::string adaptCase(const std::string& replacement, const std::string& original) const;
    
    std::string expandCStyleEscapes(const std::string& input) const;
    
    static std::string toLowerCase(const std::string& str);
    static std::string toUpperCase(const std::string& str);
    
private:
    enum class CaseType {
        NONE,
        LOWER,
        UPPER, 
        MIXED
    };
    
    const FartConfig& config_;
    std::string find_string_normalized_;
    std::string replace_string_lower_;
    std::string replace_string_upper_;
    
    CaseType analyzeCaseType(const std::string& text) const;
    bool isWordChar(char c) const;
    std::string normalizeForComparison(const std::string& text) const;
};