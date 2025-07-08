#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include "fart_config.hpp"
#include "text_processor.hpp"

class FileProcessor {
public:
    explicit FileProcessor(FartConfig& config);
    
    struct ProcessResult {
        bool success = false;
        int matches_found = 0;
        std::string error_message;
    };
    
    using ProgressCallback = std::function<void(const std::string&)>;
    
    void setProgressCallback(ProgressCallback callback) { progress_callback_ = callback; }
    
    ProcessResult processWildcards(const std::string& wildcards);
    
    ProcessResult processFile(const std::filesystem::path& file_path);
    
    ProcessResult processDirectory(const std::filesystem::path& dir_path, 
                                   const std::string& pattern, 
                                   bool recursive = false);
    
    ProcessResult findInFile(const std::filesystem::path& file_path);
    
    ProcessResult replaceInFile(const std::filesystem::path& file_path);
    
    ProcessResult processStdin();
    
    static bool isBinaryFile(const std::filesystem::path& file_path);
    
    static bool shouldSkipDirectory(const std::string& dir_name, const FartConfig::Options& options);
    
    static std::vector<std::string> splitWildcards(const std::string& wildcards);
    
    static bool matchesPattern(const std::string& filename, const std::string& pattern);

private:
    FartConfig& config_;
    std::unique_ptr<TextProcessor> text_processor_;
    ProgressCallback progress_callback_;
    
    ProcessResult processFileContents(const std::filesystem::path& file_path);
    
    ProcessResult processFileName(const std::filesystem::path& file_path);
    
    bool createBackup(const std::filesystem::path& file_path);
    
    void updateProgress(const std::string& message);
    
    std::string readFile(const std::filesystem::path& file_path);
    
    bool writeFile(const std::filesystem::path& file_path, const std::string& content);
};