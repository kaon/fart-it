#include "text_processor.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

TextProcessor::TextProcessor(const FartConfig& config) 
    : config_(config) {
    
    find_string_normalized_ = config_.getFindString();
    
    if (config_.getOptions().c_style) {
        find_string_normalized_ = expandCStyleEscapes(find_string_normalized_);
    }
    
    if (config_.getOptions().ignore_case) {
        find_string_normalized_ = toLowerCase(find_string_normalized_);
    }
    
    if (config_.getOptions().adapt_case) {
        replace_string_lower_ = toLowerCase(config_.getReplaceString());
        replace_string_upper_ = toUpperCase(config_.getReplaceString());
    }
}

std::vector<TextProcessor::FindResult> TextProcessor::findMatches(const std::string& text) const {
    std::vector<FindResult> results;
    
    if (find_string_normalized_.empty()) {
        return results;
    }
    
    std::string search_text = normalizeForComparison(text);
    size_t pos = 0;
    
    while ((pos = search_text.find(find_string_normalized_, pos)) != std::string::npos) {
        if (config_.getOptions().whole_word) {
            if (!isWordBoundary(search_text, pos) || 
                !isWordBoundary(search_text, pos + find_string_normalized_.length())) {
                pos++;
                continue;
            }
        }
        
        FindResult result;
        result.position = pos;
        result.length = find_string_normalized_.length();
        
        if (config_.getOptions().adapt_case) {
            std::string original_match = text.substr(pos, result.length);
            result.replacement = adaptCase(config_.getReplaceString(), original_match);
        } else {
            result.replacement = config_.getReplaceString();
        }
        
        results.push_back(result);
        pos += find_string_normalized_.length();
    }
    
    return results;
}

std::string TextProcessor::processLine(const std::string& line, int& match_count) const {
    match_count = 0;
    
    if (config_.isGrepMode()) {
        match_count = countMatches(line);
        return line;
    }
    
    auto matches = findMatches(line);
    match_count = static_cast<int>(matches.size());
    
    if (matches.empty()) {
        return line;
    }
    
    std::string result;
    size_t last_pos = 0;
    
    for (const auto& match : matches) {
        result += line.substr(last_pos, match.position - last_pos);
        result += match.replacement;
        last_pos = match.position + match.length;
    }
    
    result += line.substr(last_pos);
    return result;
}

int TextProcessor::countMatches(const std::string& text) const {
    auto matches = findMatches(text);
    return static_cast<int>(matches.size());
}

bool TextProcessor::isWordBoundary(const std::string& text, size_t pos) const {
    if (pos == 0 || pos >= text.length()) {
        return true;
    }
    
    return !isWordChar(text[pos - 1]) || !isWordChar(text[pos]);
}

std::string TextProcessor::adaptCase(const std::string& replacement, const std::string& original) const {
    CaseType case_type = analyzeCaseType(original);
    
    switch (case_type) {
        case CaseType::LOWER:
            return toLowerCase(replacement);
        case CaseType::UPPER:
            return toUpperCase(replacement);
        case CaseType::MIXED:
        case CaseType::NONE:
        default:
            return replacement;
    }
}

std::string TextProcessor::expandCStyleEscapes(const std::string& input) const {
    std::string result;
    result.reserve(input.length());
    
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '\\' && i + 1 < input.length()) {
            char next = input[i + 1];
            switch (next) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'a': result += '\a'; break;
                case 'v': result += '\v'; break;
                case '\\': result += '\\'; break;
                case '\'': result += '\''; break;
                case '\"': result += '\"'; break;
                case '?': result += '\?'; break;
                case 'x': {
                    if (i + 3 < input.length()) {
                        std::string hex = input.substr(i + 2, 2);
                        try {
                            int value = std::stoi(hex, nullptr, 16);
                            result += static_cast<char>(value);
                            i += 2;
                        } catch (...) {
                            result += next;
                        }
                    } else {
                        result += next;
                    }
                    break;
                }
                default:
                    if (next >= '0' && next <= '7') {
                        std::string octal;
                        for (int j = 0; j < 3 && i + 1 + j < input.length(); ++j) {
                            char c = input[i + 1 + j];
                            if (c >= '0' && c <= '7') {
                                octal += c;
                            } else {
                                break;
                            }
                        }
                        try {
                            int value = std::stoi(octal, nullptr, 8);
                            result += static_cast<char>(value);
                            i += octal.length() - 1;
                        } catch (...) {
                            result += next;
                        }
                    } else {
                        result += next;
                    }
                    break;
            }
            ++i;
        } else {
            result += input[i];
        }
    }
    
    return result;
}

std::string TextProcessor::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string TextProcessor::toUpperCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

TextProcessor::CaseType TextProcessor::analyzeCaseType(const std::string& text) const {
    int upper_count = 0;
    int lower_count = 0;
    
    for (char c : text) {
        if (std::isupper(c)) {
            upper_count++;
        } else if (std::islower(c)) {
            lower_count++;
        }
    }
    
    if (upper_count == 0 && lower_count == 0) {
        return CaseType::NONE;
    }
    
    if (upper_count > 0 && lower_count == 0) {
        return CaseType::UPPER;
    }
    
    if (lower_count > 0 && upper_count == 0) {
        return CaseType::LOWER;
    }
    
    return CaseType::MIXED;
}

bool TextProcessor::isWordChar(char c) const {
    return std::isalnum(c) || c == '_';
}

std::string TextProcessor::normalizeForComparison(const std::string& text) const {
    if (config_.getOptions().ignore_case) {
        return toLowerCase(text);
    }
    return text;
}