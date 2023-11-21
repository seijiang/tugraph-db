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
#include <vector>
#include "cypher/execution_plan/optimization/rewrite/node.h"
#include "parser/data_typedef.h"
#include "cypher/execution_plan/optimization/rewrite/path.h"
namespace cypher {
namespace rewrite {
class Node;

class Edge {
 public:
    size_t m_id;
    size_t m_source_id;
    size_t m_target_id;
    std::set<int> m_labels;
    parser::LinkDirection m_direction;

    int m_min_hop;
    int m_max_hop;
    int m_extend_id = -1;
    bool m_is_varlen = false;
    std::vector<Path *> m_paths;

    Edge() {}
    Edge(size_t id, size_t source_id, size_t target_id, const std::set<int> &labels,
         parser::LinkDirection direction)
        : m_id(id),
          m_source_id(source_id),
          m_target_id(target_id),
          m_labels(labels),
          m_direction(direction),
          m_min_hop(-1),
          m_max_hop(-1) {}

    Edge(size_t id, size_t source_id, size_t target_id, const std::set<int> &labels,
         parser::LinkDirection direction, int extend_id)
        : m_id(id),
          m_source_id(source_id),
          m_target_id(target_id),
          m_labels(labels),
          m_direction(direction),
          m_min_hop(-1),
          m_max_hop(-1),
          m_extend_id(extend_id),
          m_is_varlen(true) {}

    Edge(size_t id, size_t source_id, size_t target_id, const std::set<int> &labels,
         parser::LinkDirection direction, int min_hop, int max_hop)
        : m_id(id),
          m_source_id(source_id),
          m_target_id(target_id),
          m_labels(labels),
          m_direction(direction),
          m_min_hop(min_hop),
          m_max_hop(max_hop) {
        if (m_min_hop != -1 && m_max_hop != -1) {
            m_is_varlen = true;
        }
    }
    size_t GetSrcId() {
        switch (m_direction) {
        case parser::LinkDirection::LEFT_TO_RIGHT:
            return m_source_id;
        case parser::LinkDirection::RIGHT_TO_LEFT:
            return m_target_id;
        default:
            throw lgraph::CypherException("Failed to get src node.");
        }
    }

    size_t GetTarId() {
        switch (m_direction) {
        case parser::LinkDirection::LEFT_TO_RIGHT:
            return m_target_id;
        case parser::LinkDirection::RIGHT_TO_LEFT:
            return m_source_id;
        default:
            throw lgraph::CypherException("Failed to get src node.");
        }
    }
    ~Edge(){}
};
};  // namespace rewrite
};  // namespace cypher
