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

namespace cypher {
typedef std::map<cypher::NodeID, std::string> SchemaNodeMap;
typedef std::map<cypher::RelpID, std::tuple<cypher::NodeID, cypher::NodeID, std::set<std::string>,
                                            parser::LinkDirection, int, int>>
    SchemaRelpMap;
typedef std::pair<SchemaNodeMap, SchemaRelpMap> SchemaGraphMap;
namespace rewrite {

struct EidDirection {
    size_t m_eid;
    parser::LinkDirection m_direction;
    EidDirection(size_t eid, parser::LinkDirection direction)
        : m_eid(eid), m_direction(direction) {}

    bool operator<(const EidDirection& ed2) const { return this->m_eid < ed2.m_eid; }
};

// 查询图上vid到next_vid之间的可行映射在id_map中，key为目标图上与next_vid对应的节点。value为目标图上所有可行的边
class StateInfo {
 public:
    size_t m_vid;
    size_t m_next_vid;
    size_t m_eid;
    parser::LinkDirection m_direction;
    std::map<size_t, std::set<EidDirection>> m_id_map;
    StateInfo() {}
    StateInfo(size_t vid, size_t next_vid, size_t eid, parser::LinkDirection direction,
              std::map<size_t, std::set<EidDirection>>* id_map)
        : m_vid(vid), m_next_vid(next_vid), m_eid(eid), m_direction(direction), m_id_map(*id_map) {}
    // StateInfo(size_t vid,size_t next_vid,size_t eid,size_t direction,
    // std::map<size_t,std::set<size_t>>* id_map)
    // :m_vid(vid),m_next_vid(next_vid),m_eid(eid),m_direction(direction),m_id_map(*id_map){
    // }

    void Print() {
        std::cout << "vid:" << m_vid << std::endl;
        std::cout << "m_next_vid:" << m_next_vid << std::endl;
        std::cout << "direction:" << m_direction << std::endl;
    }
};

};  // namespace rewrite
};  // namespace cypher
