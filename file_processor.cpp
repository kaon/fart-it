#include "file_processor.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>

FileProcessor::FileProcessor(FartConfig& config) 
    : config_(config), text_processor_(std::make_unique<TextProcessor>(config)) {}

FileProcessor::ProcessResult FileProcessor::processWildcards(const std::string& wildcards) {
    ProcessResult total_result;
    total_result.success = true;
    
    auto wildcard_list = splitWildcards(wildcards);
    
    for (const auto& wildcard : wildcard_list) {
        std::filesystem::path path(wildcard);
        
        if (std::filesystem::exists(path)) {
            if (std::filesystem::is_directory(path)) {
                auto result = processDirectory(path, "*", config_.getOptions().recursive);
                total_result.matches_found += result.matches_found;
                if (!result.success) {
                    total_result.success = false;
                    total_result.error_message += result.error_message + "\n";
                }
            } else {
                auto result = processFile(path);
                total_result.matches_found += result.matches_found;
                if (!result.success) {
                    total_result.success = false;
                    total_result.error_message += result.error_message + "\n";
                }
            }
        } else {
            auto parent_path = path.parent_path();
            auto filename = path.filename().string();
            
            if (parent_path.empty()) {
                parent_path = ".";
            }
            
            auto result = processDirectory(parent_path, filename, config_.getOptions().recursive);
            total_result.matches_found += result.matches_found;
            if (!result.success) {
                total_result.success = false;
                total_result.error_message += result.error_message + "\n";
            }
        }
    }
    
    return total_result;
}

FileProcessor::ProcessResult FileProcessor::processFile(const std::filesystem::path& file_path) {
    ProcessResult result;
    
    try {
        if (!std::filesystem::exists(file_path)) {
            result.error_message = "File not found: " + file_path.string();
            return result;
        }
        
        if (!config_.getOptions().binary && isBinaryFile(file_path)) {
            if (config_.getOptions().verbose) {
                std::cerr << "Skipping binary file: " << file_path << std::endl;
            }
            result.success = true;
            return result;
        }
        
        updateProgress(file_path.string());
        
        if (config_.getOptions().filename_mode) {
            return processFileName(file_path);
        } else {
            return processFileContents(file_path);
        }
        
    } catch (const std::exception& e) {
        result.error_message = "Error processing file " + file_path.string() + ": " + e.what();
        return result;
    }
}

FileProcessor::ProcessResult FileProcessor::processDirectory(const std::filesystem::path& dir_path, 
                                                           const std::string& pattern, 
                                                           bool recursive) {
    ProcessResult total_result;
    total_result.success = true;
    
    try {
        if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
            total_result.error_message = "Directory not found: " + dir_path.string();
            return total_result;
        }
        
        std::filesystem::directory_iterator dir_iter(dir_path);
        
        for (const auto& entry : dir_iter) {
            if (entry.is_regular_file()) {
                if (matchesPattern(entry.path().filename().string(), pattern)) {
                    auto result = processFile(entry.path());
                    total_result.matches_found += result.matches_found;
                    if (!result.success) {
                        total_result.success = false;
                        total_result.error_message += result.error_message + "\n";
                    }
                }
            } else if (entry.is_directory() && recursive) {
                std::string dir_name = entry.path().filename().string();
                if (!shouldSkipDirectory(dir_name, config_.getOptions())) {
                    auto result = processDirectory(entry.path(), pattern, recursive);
                    total_result.matches_found += result.matches_found;
                    if (!result.success) {
                        total_result.success = false;
                        total_result.error_message += result.error_message + "\n";
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        total_result.success = false;
        total_result.error_message = "Error processing directory " + dir_path.string() + ": " + e.what();
    }
    
    return total_result;
}

FileProcessor::ProcessResult FileProcessor::findInFile(const std::filesystem::path& file_path) {
    ProcessResult result;
    
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            result.error_message = "Could not open file: " + file_path.string();
            return result;
        }
        
        std::string line;
        int line_number = 0;
        bool first_match = true;
        
        while (std::getline(file, line)) {
            line_number++;
            int match_count = text_processor_->countMatches(line);
            
            if (config_.getOptions().invert) {
                match_count = match_count ? 0 : 1;
            }
            
            if (match_count > 0) {
                result.matches_found += match_count;
                
                if (first_match && !config_.getOptions().count && !config_.getOptions().quiet) {
                    std::cout << file_path.string() << " :\n";
                    first_match = false;
                }
                
                if (!config_.getOptions().count) {
                    if (config_.getOptions().line_numbers) {
                        std::cout << "[" << std::setw(4) << line_number << "]";
                    }
                    std::cout << line << std::endl;
                }
            }
        }
        
        if (result.matches_found > 0) {
            config_.getStats().total_files++;
            if (config_.getOptions().count) {
                if (config_.getOptions().quiet) {
                    std::cout << file_path.string() << std::endl;
                } else {
                    std::cout << file_path.string() << " [" << result.matches_found << "]" << std::endl;
                }
            }
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.error_message = "Error reading file " + file_path.string() + ": " + e.what();
    }
    
    return result;
}

FileProcessor::ProcessResult FileProcessor::replaceInFile(const std::filesystem::path& file_path) {
    ProcessResult result;
    
    try {
        std::string content = readFile(file_path);
        std::string modified_content;
        bool file_changed = false;
        
        std::istringstream iss(content);
        std::string line;
        int line_number = 0;
        
        while (std::getline(iss, line)) {
            line_number++;
            int match_count = 0;
            std::string processed_line = text_processor_->processLine(line, match_count);
            
            if (match_count > 0) {
                result.matches_found += match_count;
                file_changed = true;
                
                if (config_.getOptions().line_numbers && !config_.getOptions().count && !config_.getOptions().quiet) {
                    std::cout << "[" << std::setw(4) << line_number << "]";
                }
            }
            
            modified_content += processed_line + "\n";
        }
        
        if (file_changed) {
            config_.getStats().total_files++;
            
            if (config_.getOptions().count && !config_.getOptions().quiet) {
                std::cout << file_path.string() << " [" << result.matches_found << "]" << std::endl;
            }
            
            if (!config_.getOptions().preview) {
                if (config_.getOptions().backup) {
                    createBackup(file_path);
                }
                
                if (!writeFile(file_path, modified_content)) {
                    result.error_message = "Could not write to file: " + file_path.string();
                    return result;
                }
            }
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.error_message = "Error processing file " + file_path.string() + ": " + e.what();
    }
    
    return result;
}

FileProcessor::ProcessResult FileProcessor::processStdin() {
    ProcessResult result;
    
    try {
        std::string line;
        int total_matches = 0;
        
        while (std::getline(std::cin, line)) {
            int match_count = 0;
            
            if (config_.isFartMode()) {
                std::string processed_line = text_processor_->processLine(line, match_count);
                std::cout << processed_line << std::endl;
            } else {
                match_count = text_processor_->countMatches(line);
                if (config_.getOptions().invert) {
                    match_count = match_count ? 0 : 1;
                }
                
                if (match_count > 0) {
                    std::cout << line << std::endl;
                }
            }
            
            total_matches += match_count;
        }
        
        result.matches_found = total_matches;
        result.success = true;
        
    } catch (const std::exception& e) {
        result.error_message = "Error processing stdin: " + std::string(e.what());
    }
    
    return result;
}

bool FileProcessor::isBinaryFile(const std::filesystem::path& file_path) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        constexpr size_t SAMPLE_SIZE = 1024;
        char buffer[SAMPLE_SIZE];
        
        file.read(buffer, SAMPLE_SIZE);
        std::streamsize bytes_read = file.gcount();
        
        size_t non_ascii_count = 0;
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            unsigned char c = static_cast<unsigned char>(buffer[i]);
            if (c == 0 || (c < 32 && c != 9 && c != 10 && c != 13)) {
                non_ascii_count++;
            }
        }
        
        return (non_ascii_count * 20 >= static_cast<size_t>(bytes_read));
        
    } catch (...) {
        return false;
    }
}

bool FileProcessor::shouldSkipDirectory(const std::string& dir_name, const FartConfig::Options& options) {
    if (options.cvs && dir_name == "CVS") return true;
    if (options.svn && dir_name == ".svn") return true;
    if (options.git && dir_name == ".git") return true;
    return false;
}

std::vector<std::string> FileProcessor::splitWildcards(const std::string& wildcards) {
    std::vector<std::string> result;
    std::stringstream ss(wildcards);
    std::string item;
    
    while (std::getline(ss, item, FartConfig::WILDCARD_SEPARATOR)) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

bool FileProcessor::matchesPattern(const std::string& filename, const std::string& pattern) {
    if (pattern == "*") {
        return true;
    }
    
    try {
        std::string regex_pattern = pattern;
        std::replace(regex_pattern.begin(), regex_pattern.end(), '*', '.');
        regex_pattern = std::regex_replace(regex_pattern, std::regex("\\."), ".*");
        regex_pattern = std::regex_replace(regex_pattern, std::regex("\\?"), ".");
        
        std::regex regex(regex_pattern);
        return std::regex_match(filename, regex);
    } catch (...) {
        return filename == pattern;
    }
}

FileProcessor::ProcessResult FileProcessor::processFileContents(const std::filesystem::path& file_path) {
    if (config_.isGrepMode()) {
        return findInFile(file_path);
    } else {
        return replaceInFile(file_path);
    }
}

FileProcessor::ProcessResult FileProcessor::processFileName(const std::filesystem::path& file_path) {
    ProcessResult result;
    
    std::string filename = file_path.filename().string();
    int match_count = 0;
    std::string new_filename = text_processor_->processLine(filename, match_count);
    
    if (match_count > 0) {
        result.matches_found = match_count;
        config_.getStats().total_files++;
        
        if (config_.isFartMode() && !config_.getOptions().preview) {
            auto new_path = file_path.parent_path() / new_filename;
            
            try {
                std::filesystem::rename(file_path, new_path);
                std::cout << file_path.string() << " => " << new_filename << std::endl;
            } catch (const std::exception& e) {
                result.error_message = "Could not rename " + file_path.string() + " to " + new_filename + ": " + e.what();
                return result;
            }
        } else {
            std::cout << file_path.string() << std::endl;
        }
    }
    
    result.success = true;
    return result;
}

bool FileProcessor::createBackup(const std::filesystem::path& file_path) {
    try {
        auto backup_path = file_path.string() + FartConfig::BACKUP_SUFFIX;
        std::filesystem::copy_file(file_path, backup_path, std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (...) {
        return false;
    }
}

void FileProcessor::updateProgress(const std::string& message) {
    if (progress_callback_) {
        progress_callback_(message);
    }
}

std::string FileProcessor::readFile(const std::filesystem::path& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + file_path.string());
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

bool FileProcessor::writeFile(const std::filesystem::path& file_path, const std::string& content) {
    try {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        return file.good();
    } catch (...) {
        return false;
    }
}