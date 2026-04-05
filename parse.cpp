//
// Created by zardoru on 05-04-26.
//

#include "parse.h"

#include <boost/json/value.hpp>
#include <boost/algorithm/string.hpp>

/* taken from q2pro */
/*
==============
COM_Parse

Parse a token out of a string.
Handles C and C++ comments.
==============
*/
std::optional<parse_result_t> q2parser_t::parse(std::string input) {
    parse_result_t result;
    int c;

    auto data = input.begin();

    if (input.empty())
        return std::nullopt;

    // skip whitespace
skipwhite:
    while ((c = *data) <= ' ') {
        if (c == 0) {
            result.rest = std::string(data, input.end());
            if (result.token.empty())
                return std::nullopt;

            return result;
        }

        if (c == '\n')
            linenum++;

        ++data;
    }

    // skip // comments
    if (c == '/' && data[1] == '/') {
        data += 2;

        while (*data && *data != '\n')
            ++data;

        goto skipwhite;
    }

    // skip /* */ comments
    if (c == '/' && data[1] == '*') {
        data += 2;
        while (*data) {
            if (data[0] == '*' && data[1] == '/') {
                data += 2;
                break;
            }

            if (data[0] == '\n')
                linenum++;

            ++data;
        }
        goto skipwhite;
    }

    // handle quoted strings specially
    if (c == '\"') {
        ++data;
        while (true) {
            c = *data++;
            if (c == '\"' || !c) {
                goto finish;
            }
            if (c == '\n') {
                linenum++;
            }

            result.token += c;
        }
    }

    // parse a regular word
    do {
        result.token += c;
        ++data;
        c = *data;
    } while (c > 32);

finish:
    if (result.token.empty())
        return std::nullopt;

    result.rest = std::string(data, input.end());
    return result;
}

std::expected<edict_list_t, parse_error> q2parser_t::parse_edicts(std::string input) {
    edict_list_t result;
    auto parse_state = parse(input);

    while (parse_state.has_value() && parse_state->token == "{") {
        edict_parse_t edict;

        parse_state = parse(parse_state->rest);
        while (parse_state.has_value() && parse_state->token != "}") {
            auto key = parse_state->token;

            parse_state = parse(parse_state->rest);
            if (parse_state.has_value()) {
                auto value = parse_state->token;

                if (value == "}")
                    return std::unexpected(parse_error(std::format("unexpected '}}' in edict")));

                edict.emplace(boost::to_lower_copy(key), value);
                parse_state = parse(parse_state->rest);
            }
        }

        result.emplace_back(edict);
        if (parse_state.has_value())
            parse_state = parse(parse_state->rest);
    }

    return result;
}
