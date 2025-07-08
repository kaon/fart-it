#include "argument_parser.hpp"
#include <iostream>
#include <iomanip>

ArgumentParser::ArgumentParser() {
    initializeArguments();
}

ArgumentParser::ParseResult ArgumentParser::parse(int argc, char* argv[], FartConfig& config) {
    ParseResult result;
    result.success = true;
    
    auto& options = config.getOptions();
    bool parsing_options = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (parsing_options && arg.length() > 1 && arg[0] == '-') {
            if (arg == "--") {
                parsing_options = false;
                continue;
            }
            
            if (arg.length() > 2 && arg.substr(0, 2) == "--") {
                std::string long_option = arg.substr(2);
                auto parse_result = parseLongOption(long_option, options);
                if (!parse_result.success) {
                    return parse_result;
                }
                if (parse_result.show_help) {
                    result.show_help = true;
                }
            } else {
                std::string short_options = arg.substr(1);
                auto parse_result = parseShortOptions(short_options, options);
                if (!parse_result.success) {
                    return parse_result;
                }
                if (parse_result.show_help) {
                    result.show_help = true;
                }
            }
        } else {
            if (!config.hasWildcard()) {
                config.setWildcard(arg);
            } else if (!config.hasFindString()) {
                config.setFindString(arg);
            } else if (!config.hasReplaceString()) {
                config.setReplaceString(arg);
            } else {
                result.success = false;
                result.error_message = "Too many arguments: " + arg;
                return result;
            }
        }
    }
    
    if (options.help || !config.hasWildcard()) {
        result.show_help = true;
    }
    
    if (options.remove && config.hasReplaceString()) {
        result.success = false;
        result.error_message = "Option --remove conflicts with replace_string";
        return result;
    }
    
    if (options.remove && config.hasFindString()) {
        config.setReplaceString("");
    }
    
    if (options.count && options.line_numbers) {
        std::cerr << "Warning: conflicting options: --line-number, --count" << std::endl;
    }
    
    return result;
}

void ArgumentParser::showUsage() const {
    std::cout << "\nFind And Replace Text " << std::left << std::setw(30) << FartConfig::VERSION 
              << "by Lionello Lunesu\n\n";
    
    std::cout << "Usage: fart [options] [--] <wildcard>[" << FartConfig::WILDCARD_SEPARATOR 
              << "...] [find_string] [replace_string]\n";
    
    std::cout << "\nOptions:\n";
    
    for (const auto& arg : argument_definitions_) {
        if (arg.description.empty()) {
            continue;
        }
        
        if (arg.short_option != ' ') {
            std::cout << " -" << arg.short_option << ",";
        } else {
            std::cout << "    ";
        }
        
        std::cout << " --" << std::left << std::setw(14) << arg.long_option 
                  << arg.description << "\n";
    }
    
    std::cout << std::endl;
}

void ArgumentParser::showVersion() const {
    std::cout << "fart " << FartConfig::VERSION << std::endl;
}

void ArgumentParser::initializeArguments() {
    argument_definitions_ = {
        {'h', "help", "Show this help message (ignores other options)", nullptr},
        {'q', "quiet", "Suppress output to stdio / stderr", nullptr},
        {'V', "verbose", "Show more information", nullptr},
        {'r', "recursive", "Process sub-folders recursively", nullptr},
        {'c', "count", "Only show filenames, match counts and totals", nullptr},
        {'i', "ignore-case", "Case insensitive text comparison", nullptr},
        {'v', "invert", "Print lines NOT containing the find string", nullptr},
        {'n', "line-number", "Print line number before each line (1-based)", nullptr},
        {'w', "word", "Match whole word (uses C syntax, like grep)", nullptr},
        {'f', "filename", "Find (and replace) filename instead of contents", nullptr},
        {'B', "binary", "Also search (and replace) in binary files (CAUTION)", nullptr},
        {'C', "c-style", "Allow C-style extended characters (\\xFF\\0\\t\\n\\r\\\\ etc.)", nullptr},
        {' ', "cvs", "Skip cvs dirs; execute \"cvs edit\" before changing files", nullptr},
        {' ', "svn", "Skip svn dirs", nullptr},
        {' ', "git", "Skip git dirs (default)", nullptr},
        {' ', "remove", "Remove all occurences of the find_string", nullptr},
        {'a', "adapt", "Adapt the case of replace_string to found string", nullptr},
        {'b', "backup", "Make a backup of each changed file", nullptr},
        {'p', "preview", "Do not change the files but print the changes", nullptr}
    };
    
    for (auto& arg : argument_definitions_) {
        if (arg.short_option != ' ') {
            short_options_[arg.short_option] = arg.flag_ptr;
        }
        long_options_[arg.long_option] = arg.flag_ptr;
    }
}

ArgumentParser::ParseResult ArgumentParser::parseShortOptions(const std::string& options, FartConfig::Options& config_options) {
    ParseResult result;
    result.success = true;
    
    for (char option : options) {
        if (!isValidOption(option)) {
            if (option != '?') {
                result.error_message = "Invalid option: -" + std::string(1, option);
                result.success = false;
                return result;
            }
            result.show_help = true;
            continue;
        }
        
        switch (option) {
            case 'h': config_options.help = true; result.show_help = true; break;
            case 'q': config_options.quiet = true; break;
            case 'V': config_options.verbose = true; break;
            case 'r': config_options.recursive = true; break;
            case 'c': config_options.count = true; break;
            case 'i': config_options.ignore_case = true; break;
            case 'v': config_options.invert = true; break;
            case 'n': config_options.line_numbers = true; break;
            case 'w': config_options.whole_word = true; break;
            case 'f': config_options.filename_mode = true; break;
            case 'B': config_options.binary = true; break;
            case 'C': config_options.c_style = true; break;
            case 'a': config_options.adapt_case = true; break;
            case 'b': config_options.backup = true; break;
            case 'p': config_options.preview = true; break;
        }
    }
    
    return result;
}

ArgumentParser::ParseResult ArgumentParser::parseLongOption(const std::string& option, FartConfig::Options& config_options) {
    ParseResult result;
    result.success = true;
    
    if (!isValidLongOption(option)) {
        result.error_message = "Invalid option: --" + option;
        result.success = false;
        return result;
    }
    
    if (option == "help") { config_options.help = true; result.show_help = true; }
    else if (option == "quiet") { config_options.quiet = true; }
    else if (option == "verbose") { config_options.verbose = true; }
    else if (option == "recursive") { config_options.recursive = true; }
    else if (option == "count") { config_options.count = true; }
    else if (option == "ignore-case") { config_options.ignore_case = true; }
    else if (option == "invert") { config_options.invert = true; }
    else if (option == "line-number") { config_options.line_numbers = true; }
    else if (option == "word") { config_options.whole_word = true; }
    else if (option == "filename") { config_options.filename_mode = true; }
    else if (option == "binary") { config_options.binary = true; }
    else if (option == "c-style") { config_options.c_style = true; }
    else if (option == "cvs") { config_options.cvs = true; }
    else if (option == "svn") { config_options.svn = true; }
    else if (option == "git") { config_options.git = true; }
    else if (option == "remove") { config_options.remove = true; }
    else if (option == "adapt") { config_options.adapt_case = true; }
    else if (option == "backup") { config_options.backup = true; }
    else if (option == "preview") { config_options.preview = true; }
    
    return result;
}

bool ArgumentParser::isValidOption(char option) const {
    return short_options_.find(option) != short_options_.end();
}

bool ArgumentParser::isValidLongOption(const std::string& option) const {
    return long_options_.find(option) != long_options_.end();
}