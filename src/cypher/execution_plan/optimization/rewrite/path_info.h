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
#include "execution_plan/execution_plan.h"
#include "cypher/execution_plan/optimization/rewrite/path.h"
// #include "parser/data_typedef.h"
#include "graph/common.h"
#include "cypher/execution_plan/optimization/rewrite/state_info.h"

namespace cypher::rewrite {

class PathInfo {
 public:
    cypher::SchemaNodeMap m_schema_node_map;
    cypher::SchemaRelpMap m_schema_relp_map;
    std::map<cypher::RelpID, std::vector<Path>> m_var_len_paths_map;
    PathInfo() {}
    PathInfo(cypher::SchemaNodeMap* schema_node_map, cypher::SchemaRelpMap* schema_relp_map);
    ~PathInfo() {}
    // bool operator==(const PathInfo& other);
    bool CheckNewPathInfo(cypher::SchemaNodeMap* snm, cypher::SchemaRelpMap* srm);
    void AddPath(cypher::RelpID& rid, Path*& path);
    std::string ToString();
};

};  // namespace cypher::rewrite
