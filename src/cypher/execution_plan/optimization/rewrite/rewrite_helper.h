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
#include <unordered_map>
#include <vector>
#include "cypher/execution_plan/optimization/rewrite/node.h"
#include "cypher/execution_plan/optimization/rewrite/graph.h"
#include "cypher/execution_plan/optimization/rewrite/schema_rewrite.h"

#include "cypher/execution_plan/ops/op.h"
#include "graph/common.h"
#include "parser/data_typedef.h"
#include "cypher/execution_plan/optimization/rewrite/state_info.h"
#include "execution_plan/execution_plan.h"
#include "cypher/execution_plan/optimization/rewrite/path_info.h"

namespace cypher::rewrite {

class RewriteHelper {
 public:
    Graph target_graph;
    Graph query_graph;
    std::map<std::string, int> label2idx;  // 每个label对应一个id
    int label_cnt = 0;
    std::vector<std::string> idx2label;  // id到label的对应

    std::map<size_t, cypher::NodeID> vidx2pidx;  // 顶点id到pattern graph中id对应
    std::map<cypher::NodeID, size_t> pidx2vidx;  // pattern graph中id到顶点id对应
    std::map<size_t, cypher::NodeID> eidx2pidx;  // 边id到pattern graph中id对应

    std::vector<cypher::SchemaGraphMap> sgm;
    cypher::SchemaNodeMap* m_schema_node_map;
    cypher::SchemaRelpMap* m_schema_relp_map;
    std::vector<PathInfo> path_infos;

    bool need_rebuild = false;

    bool skip_rewrite = false;

    RewriteHelper(const lgraph::SchemaInfo& schema_info, cypher::SchemaNodeMap* schema_node_map,
                  cypher::SchemaRelpMap* schema_relp_map);
    std::vector<PathInfo> GetEffectivePath();
    int GetLabelNum(const std::string& label);
    ~RewriteHelper(){}
};

};  // namespace cypher::rewrite
