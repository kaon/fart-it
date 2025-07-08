#include <iostream>
#include <memory>
#include <string>

#include "fart_config.hpp"
#include "argument_parser.hpp"
#include "file_processor.hpp"

class FartApplication {
public:
    int run(int argc, char* argv[]) {
        ArgumentParser parser;
        auto parse_result = parser.parse(argc, argv, config_);
        
        if (parse_result.show_help) {
            parser.showUsage();
            return parse_result.success ? 0 : -1;
        }
        
        if (!parse_result.success) {
            std::cerr << "Error: " << parse_result.error_message << std::endl;
            return -1;
        }
        
        if (config_.isFindMode()) {
            return handleFindMode();
        }
        
        if (config_.isGrepMode()) {
            return handleGrepMode();
        }
        
        if (config_.isFartMode()) {
            return handleFartMode();
        }
        
        std::cerr << "Error: Invalid mode" << std::endl;
        return -1;
    }

private:
    FartConfig config_;
    
    int handleFindMode() {
        FileProcessor processor(config_);
        
        if (config_.getOptions().verbose) {
            processor.setProgressCallback([](const std::string& file) {
                std::cerr << "Processing: " << file << std::endl;
            });
        }
        
        auto result = processor.processWildcards(config_.getWildcard());
        
        if (!result.success) {
            std::cerr << "Error: " << result.error_message << std::endl;
            return -1;
        }
        
        if (!config_.getOptions().quiet) {
            std::cout << "Found " << config_.getStats().total_files << " file(s)." << std::endl;
        }
        
        return config_.getStats().total_files;
    }
    
    int handleGrepMode() {
        FileProcessor processor(config_);
        
        if (config_.getOptions().verbose) {
            processor.setProgressCallback([](const std::string& file) {
                std::cerr << "Searching: " << file << std::endl;
            });
        }
        
        FileProcessor::ProcessResult result;
        
        if (config_.getWildcard() == "-") {
            result = processor.processStdin();
        } else {
            result = processor.processWildcards(config_.getWildcard());
        }
        
        if (!result.success) {
            std::cerr << "Error: " << result.error_message << std::endl;
            return -1;
        }
        
        if (!config_.getOptions().quiet) {
            std::cout << "Found " << config_.getStats().total_matches 
                      << " occurrence(s) in " << config_.getStats().total_files 
                      << " file(s)." << std::endl;
        }
        
        return config_.getStats().total_matches;
    }
    
    int handleFartMode() {
        auto& options = config_.getOptions();
        
        if (options.binary && !options.preview && !options.backup) {
            std::cerr << "Error: too dangerous; must specify --backup when using --binary" << std::endl;
            return -2;
        }
        
        if (options.binary && !options.preview) {
            std::cerr << "Warning: fart may corrupt binary files" << std::endl;
        }
        
        if ((options.cvs || options.svn) && options.filename_mode) {
            std::cerr << "Error: renaming version controlled files would destroy their history" << std::endl;
            return -3;
        }
        
        FileProcessor processor(config_);
        
        if (options.verbose) {
            processor.setProgressCallback([](const std::string& file) {
                std::cerr << "Processing: " << file << std::endl;
            });
        }
        
        FileProcessor::ProcessResult result;
        
        if (config_.getWildcard() == "-") {
            result = processor.processStdin();
        } else {
            result = processor.processWildcards(config_.getWildcard());
        }
        
        if (!result.success) {
            std::cerr << "Error: " << result.error_message << std::endl;
            return -1;
        }
        
        if (!options.quiet) {
            std::cout << "Replaced " << config_.getStats().total_matches 
                      << " occurrence(s) in " << config_.getStats().total_files 
                      << " file(s)." << std::endl;
        }
        
        return config_.getStats().total_matches;
    }
};

int main(int argc, char* argv[]) {
    try {
        FartApplication app;
        return app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception" << std::endl;
        return -1;
    }
}