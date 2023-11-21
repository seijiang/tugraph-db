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

#include "cypher/execution_plan/ops/op.h"
#include "graph/common.h"
#include "parser/data_typedef.h"
#include "cypher/execution_plan/optimization/rewrite/state_info.h"
#include "execution_plan/execution_plan.h"
#include "cypher/execution_plan/optimization/rewrite/path_info.h"
// #include "cypher/filter/filter.h"
// #include "cypher/graph/graph.h"
// #include "cypher/parser/clause.h"
// #include "lgraph/lgraph.h"

namespace cypher::rewrite {

class SchemaRewrite {
 public:
    std::map<std::string, int>* label2idx;  // 每个label对应一个id
    int label_cnt = 0;
    std::vector<std::string>* idx2label;  // id到label的对应

    std::map<size_t, cypher::NodeID>* vidx2pidx;  // 顶点id到pattern graph中id对应
    std::map<cypher::NodeID, size_t>* pidx2vidx;  // pattern graph中id到顶点id对应
    std::map<size_t, cypher::NodeID>* eidx2pidx;  // 边id到pattern graph中id对应

    std::vector<int> m_direction_core;  // 记录映射之中的位置关系，0为出边，1为入边
    std::vector<std::set<EidDirection>> edge_core;  // 保存查询图中边id到目标图中边id的对应关系

    std::vector<StateInfo> edge_stateinfo_core;  // 保存查询图中边id到目标图中边id的对应关系
    std::vector<cypher::SchemaGraphMap> sgm;
    cypher::SchemaNodeMap* m_schema_node_map;
    cypher::SchemaRelpMap* m_schema_relp_map;
    std::vector<PathInfo>* m_path_infos;
    PathInfo* m_path_info = nullptr;

    std::vector<bool> visited;
    std::vector<bool> edge_visited;

    Graph* target_graph;
    Graph* query_graph;
    Graph* original_graph;
    std::vector<int> core_2;  // 保存查询图中点id到目标图中点id的对应关系
    size_t query_size;
    size_t target_size;
    size_t map_cnt = 0;  // 查询图中点已经匹配的个数
    size_t depth = 0;    // 遍历的深度
    void GetEffectivePath();

    SchemaRewrite(std::map<std::string, int>* label2idx, std::vector<std::string>* idx2label,
                  std::map<size_t, cypher::NodeID>* vidx2pidx,
                  std::map<cypher::NodeID, size_t>* pidx2vidx,
                  std::map<size_t, cypher::NodeID>* eidx2pidx,
                  cypher::SchemaNodeMap* m_schema_node_map,
                  cypher::SchemaRelpMap* m_schema_relp_map, std::vector<PathInfo>* path_infos);
    ~SchemaRewrite() {}

    std::vector<cypher::SchemaGraphMap> GetEffectivePath(Graph* target_graph, Graph* query_graph);
    // int GetLabelNum(const std::string& label);
    void PrintLabel2Idx();
    void Printidx2pidx();
    // void PrintMapping();
    void PrintVarLenMapping();
    // void AddMapping();
    void AddVarLenMapping();
    void Reset();
    bool CheckNodeLabel(size_t vid, size_t t_vid);
    // void Backtrack(size_t vid);
    void GetNextTVidByEIdDirection(std::map<size_t, std::set<EidDirection>>&,
                                   std::set<EidDirection>& edge_ids, size_t t_vid);
    void GetNextEdgeIdDirections(std::set<EidDirection>&, Edge* edge, size_t t_vid,
                                 size_t direction);
    void MatchRecursive(size_t vid, size_t t_vid);
    void GenCandidateStateInfo(std::vector<StateInfo>& candidate_state_infos);
    int GetPathInfoId(cypher::SchemaNodeMap* snm, cypher::SchemaRelpMap* srm);
};

// SchemaRewrite(/* args */)

// ~SchemaRewrite()
};  // namespace cypher::rewrite
