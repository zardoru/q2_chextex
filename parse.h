#pragma once
#include <expected>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

constexpr size_t MAX_TOKEN_CHARS = 1024;

struct parse_result_t {
    std::string token;
    std::string rest;
};

typedef std::unordered_map<std::string, std::string> edict_parse_t;
typedef std::vector<edict_parse_t> edict_list_t;

class parse_error : public std::runtime_error {
public:
    parse_error(const std::string& message) : std::runtime_error(message) {}
};

class q2parser_t {
    int linenum = 0;
public:
    std::optional<parse_result_t> parse(std::string input);

    std::expected<edict_list_t, parse_error> parse_edicts(std::string input);

    int get_linenum() const { return linenum; }
};
