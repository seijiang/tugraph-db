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

#include "cypher/execution_plan/optimization/rewrite/path.h"

namespace cypher::rewrite {
void Path::append(std::set<std::string>& elabels, std::string& src_label, std::string& dst_label,
                  parser::LinkDirection direction) {
    this->m_vlabels.emplace_back(src_label);
    this->m_vlabels.emplace_back(dst_label);
    this->m_elabels.emplace_back(elabels);
    this->m_direction.emplace_back(direction);
}
void Path::append(std::set<std::string>& elabels, std::string& dst_label,
                  parser::LinkDirection direction) {
    this->m_vlabels.emplace_back(dst_label);
    this->m_elabels.emplace_back(elabels);
    this->m_direction.emplace_back(direction);
}

void Path::append(std::string& elabel, std::string& src_label, std::string& dst_label,
                  parser::LinkDirection direction) {
    this->m_vlabels.emplace_back(src_label);
    this->m_vlabels.emplace_back(dst_label);
    this->m_elabels_str.emplace_back(elabel);
    this->m_direction.emplace_back(direction);
}
void Path::append(std::string& elabel, std::string& dst_label, parser::LinkDirection direction) {
    this->m_vlabels.emplace_back(dst_label);
    this->m_elabels_str.emplace_back(elabel);
    this->m_direction.emplace_back(direction);
}
std::string Path::ToString() {
    if (m_vlabels.empty()) {
        return "";
    }
    std::string str("(");
    str.append(m_vlabels[0]);
    str.append(")");
    for (size_t i = 1; i < m_vlabels.size(); i++) {
        str.append("-[");
        str.append(m_elabels_str[i - 1]);
        str.append("]-(");
        str.append(m_vlabels[i]);
        str.append(")");
    }
    return str;
}
std::string Path::ToStringV1() {
    if (m_vlabels.empty()) {
        return "";
    }
    std::string str("(");
    str.append(m_vlabels[0]);
    str.append(")");
    for (size_t i = 1; i < m_vlabels.size(); i++) {
        auto direction = m_direction[i - 1];
        if (direction == parser::LinkDirection::LEFT_TO_RIGHT) {
            str.append("-[");

            std::string label_str("");
            std::set<std::string> elabels = m_elabels[i - 1];
            for (auto elabel : elabels) {
                label_str.append(elabel);
                label_str.append(".");
            }

            str.append(label_str);
            str.append("]->(");
            str.append(m_vlabels[i]);
            str.append(")");
        } else if (direction == parser::LinkDirection::RIGHT_TO_LEFT) {
            str.append("<-[");

            std::string label_str("");
            std::set<std::string> elabels = m_elabels[i - 1];
            for (auto elabel : elabels) {
                label_str.append(elabel);
                label_str.append(".");
            }

            str.append(label_str);
            str.append("]-(");
            str.append(m_vlabels[i]);
            str.append(")");
        } else {
            str.append("-[");

            std::string label_str("");
            std::set<std::string> elabels = m_elabels[i - 1];
            for (auto elabel : elabels) {
                label_str.append(elabel);
                label_str.append(".");
            }

            str.append(label_str);
            str.append("]-(");
            str.append(m_vlabels[i]);
            str.append(")");
        }
    }
    return str;
}
}  // namespace cypher::rewrite
