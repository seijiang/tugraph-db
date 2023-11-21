/**
 * Copyright 2022 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once
#include <iostream>
#include <vector>
#include "parser/data_typedef.h"
// #include "cypher/execution_plan/ops/op.h"

namespace cypher::rewrite {
class Path {
 public:
    std::vector<std::string> m_vlabels;
    std::vector<std::set<std::string>> m_elabels;
    std::vector<std::string> m_elabels_str;
    std::vector<parser::LinkDirection> m_direction;
    Path(/* args */) {}
    ~Path() {}
    void append(std::set<std::string>& elabels, std::string& src_label, std::string& dst_label,
                parser::LinkDirection direction);
    void append(std::set<std::string>& elabels, std::string& dst_label,
                parser::LinkDirection direction);

    void append(std::string& elabel, std::string& src_label, std::string& dst_label,
                parser::LinkDirection direction);
    void append(std::string& elabel, std::string& dst_label, parser::LinkDirection direction);
    std::string ToString();
    std::string ToStringV1();

    bool operator==(const Path& other) {
        if ((this->m_vlabels == other.m_vlabels) && (this->m_elabels == other.m_elabels) &&
            (this->m_direction == other.m_direction)) {
            return true;
        }
        return false;
    }
};

}  // namespace cypher::rewrite
