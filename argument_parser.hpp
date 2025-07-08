#pragma once

#include <string>
#include <vector>
#include <map>
#include "fart_config.hpp"

class ArgumentParser {
public:
    struct ArgumentDefinition {
        char short_option;
        std::string long_option;
        std::string description;
        bool* flag_ptr;
    };
    
    ArgumentParser();
    
    struct ParseResult {
        bool success = false;
        bool show_help = false;
        std::string error_message;
    };
    
    ParseResult parse(int argc, char* argv[], FartConfig& config);
    
    void showUsage() const;
    
    void showVersion() const;

private:
    std::vector<ArgumentDefinition> argument_definitions_;
    std::map<char, bool*> short_options_;
    std::map<std::string, bool*> long_options_;
    
    void initializeArguments();
    
    ParseResult parseShortOptions(const std::string& options, FartConfig::Options& config_options);
    
    ParseResult parseLongOption(const std::string& option, FartConfig::Options& config_options);
    
    bool isValidOption(char option) const;
    
    bool isValidLongOption(const std::string& option) const;
};