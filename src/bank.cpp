/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
 * Copyright (C) 2023  Patrik Ehringer
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JS80P__BANK_CPP
#define JS80P__BANK_CPP

#include <algorithm>
#include <cstring>

#include "bank.hpp"

#include "programs.cpp"


namespace JS80P
{

Bank::Program::Program(
        std::string const& name,
        std::string const& default_name,
        std::string const& serialized
) : name(""), default_name(""), serialized(""), params_start(0)
{
    this->default_name = sanitize(default_name);
    import_without_update(serialized);
    set_name(name);
}


void Bank::Program::set_name(std::string const& new_name)
{
    set_name_without_update(new_name);
    update();
}


void Bank::Program::set_name_without_update(std::string const& new_name)
{
    name = sanitize(new_name);
}


std::string Bank::Program::sanitize(std::string const& name) const
{
    constexpr Integer buffer_size = NAME_MAX_LENGTH + 2;

    char filtered[buffer_size];
    Integer next = 0;

    std::fill_n(filtered, buffer_size, '\x00');

    for (std::string::const_iterator it = name.begin(); it != name.end(); ++it) {
        if (is_allowed_char(*it) && (next > 0 || *it != ' ')) {
            filtered[next++] = *it;

            if (next > NAME_MAX_LENGTH) {
                filtered[next] = '\x00';
                break;
            }
        }
    }

    --next;

    while (next >= 0 && filtered[next] == ' ') {
        filtered[next--] = '\x00';
    }

    std::string sanitized(filtered);

    if (sanitized.length() >= NAME_MAX_LENGTH) {
        sanitized.erase(20);
        sanitized += "...";
    } else if (sanitized.length() == 0) {
        sanitized = default_name;
    }

    return sanitized;
}


bool Bank::Program::is_allowed_char(char const c) const
{
    /* printable ASCII, except for '[', '\\', and ']' */
    return c <= '~' && c >= ' ' && c != '[' && c != '\\' && c != ']';
}


void Bank::Program::update()
{
    std::string prefix("[js80p]\r\nNAME = ");
    prefix += name;
    prefix += "\r\n";

    std::string::size_type const new_params_start = prefix.length();

    serialized = prefix + serialized.substr(params_start);
    params_start = new_params_start;
}


Bank::Program::Program()
    : name(""),
    default_name(""),
    serialized(""),
    params_start(0)
{
    update();
}


Bank::Program::Program(Program const& program)
    : name(program.name),
    default_name(program.default_name),
    serialized(program.serialized),
    params_start(program.params_start)
{
}


Bank::Program::Program(Program const&& program)
    : name(program.name),
    default_name(program.default_name),
    serialized(program.serialized),
    params_start(program.params_start)
{
}


Bank::Program& Bank::Program::operator=(Program const& program)
{
    if (this != &program) {
        name = program.name;
        serialized = program.serialized;
        default_name = program.default_name;
        params_start = program.params_start;
    }

    return *this;
}


Bank::Program& Bank::Program::operator=(Program const&& program)
{
    if (this != &program) {
        name = program.name;
        serialized = program.serialized;
        default_name = program.default_name;
        params_start = program.params_start;
    }

    return *this;
}


std::string const& Bank::Program::get_name() const
{
    return name;
}


std::string const& Bank::Program::serialize() const
{
    return serialized;
}


bool Bank::Program::is_blank() const
{
    return params_start == serialized.length();
}


void Bank::Program::import(std::string const& serialized)
{
    import_without_update(serialized);
    update();
}


void Bank::Program::import_without_update(std::string const& serialized)
{
    Serializer::Lines* lines = Serializer::parse_lines(serialized);
    Serializer::Lines::const_iterator it = lines->begin();

    import_without_update(it, lines->end());

    delete lines;
}


void Bank::Program::import(
        Serializer::Lines::const_iterator& it,
        Serializer::Lines::const_iterator const& end
) {
    import_without_update(it, end);
    update();
}


void Bank::Program::import_without_update(
        Serializer::Lines::const_iterator& it,
        Serializer::Lines::const_iterator const& end
) {
    std::string program_name("");
    std::string serialized_params("");
    char section_name[8];
    char param_name[8];
    char suffix[4];
    bool is_js80p_section = false;
    bool found_program_name = false;

    for (; it != end; ++it) {
        std::string const& line = *it;
        std::string::const_iterator line_it = line.begin();
        std::string::const_iterator line_end = line.end();

        if (Serializer::parse_section_name(line, section_name)) {
            if (is_js80p_section) {
                break;
            }

            serialized_params = "";
            program_name = "";
            param_name[0] = '\x00';
            is_js80p_section = Serializer::is_js80p_section_start(section_name);
        } else if (
                is_js80p_section
                && Serializer::parse_line_until_value(
                    line_it, line_end, param_name, suffix
                )
                && strncmp(param_name, "NAME", 8) == 0
                && strncmp(suffix, "", 4) == 0
        ) {
            Serializer::skipping_remaining_whitespace_or_comment_reaches_the_end(
                line_it, line_end
            );
            program_name = &(*line_it);
            found_program_name = true;
        } else if (is_js80p_section) {
            serialized_params += line;
            serialized_params += "\r\n";
        }
    }

    if (is_js80p_section) {
        if (found_program_name) {
            set_name_without_update(program_name);
        }

        params_start = 0;
        serialized = serialized_params;
    } else {
        set_name_without_update("");
        params_start = 0;
        serialized = "";
    }
}


Bank::Bank() : current_program_index(0)
{
    reset();
}


void Bank::reset()
{
    size_t i = 0;

    current_program_index = 0;

    for (; i != NUMBER_OF_BUILT_IN_PROGRAMS; ++i) {
        programs[i] = BUILT_IN_PROGRAMS[i];
    }

    char default_name[Program::NAME_MAX_LENGTH];

    for (; i != NUMBER_OF_PROGRAMS; ++i) {
        snprintf(
            default_name,
            Program::NAME_MAX_LENGTH,
            "Blank Slot %lu", (long unsigned int)(i + 1)
        );
        default_name[Program::NAME_MAX_LENGTH - 1] = '\x00';
        programs[i] = Program("", default_name, "");
    }
}


Bank::Program& Bank::operator[](size_t const index)
{
    if (index >= NUMBER_OF_PROGRAMS) {
        return programs[NUMBER_OF_PROGRAMS - 1];
    }

    return programs[index];
}


Bank::Program const& Bank::operator[](size_t const index) const
{
    if (index >= NUMBER_OF_PROGRAMS) {
        return programs[NUMBER_OF_PROGRAMS - 1];
    }

    return programs[index];
}


size_t Bank::get_current_program_index() const
{
    return current_program_index;
}


void Bank::set_current_program_index(size_t const new_index)
{
    if (new_index >= NUMBER_OF_PROGRAMS) {
        current_program_index = NUMBER_OF_PROGRAMS - 1;
    } else {
        current_program_index = new_index;
    }
}


void Bank::import(std::string const& serialized_bank)
{
    Serializer::Lines* lines = Serializer::parse_lines(serialized_bank);
    Serializer::Lines::const_iterator it = lines->begin();
    Serializer::Lines::const_iterator end = lines->end();
    size_t next_program_index = 0;

    reset();

    while (it != end && next_program_index < NUMBER_OF_PROGRAMS) {
        programs[next_program_index++].import(it, end);
    }

    delete lines;
}


std::string Bank::serialize() const
{
    size_t non_blank_programs = 0;

    for (size_t i = 0; i != NUMBER_OF_PROGRAMS; ++i) {
        if (!programs[i].is_blank()) {
            ++non_blank_programs;
        }
    }

    std::string result;

    result.reserve(non_blank_programs * 16384);

    for (size_t i = 0; i != NUMBER_OF_PROGRAMS; ++i) {
        result += programs[i].serialize();
        result += "\r\n";
    }

    return result;
}

}

#endif
