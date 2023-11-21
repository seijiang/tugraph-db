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
#include <string>
#include <vector>
#include "cypher/execution_plan/optimization/rewrite/node.h"
#include "cypher/execution_plan/optimization/rewrite/edge.h"

#include "cypher/execution_plan/ops/op.h"
namespace cypher {
namespace rewrite {

class Graph {
 public:
    std::vector<cypher::rewrite::Node> m_nodes;
    std::vector<cypher::rewrite::Edge> m_edges;
    size_t m_edge_cnt = 0;
    size_t m_node_cnt = 0;

    Graph(/* args */) {}
    ~Graph() {}
    void AddNode(size_t id, int label);

    void AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int> &labels,
                 parser::LinkDirection direction);
    void AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int> &labels,
                 parser::LinkDirection direction, int min_hop, int max_hop);
    void AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int> &labels,
                 parser::LinkDirection direction, size_t extend_id);
    void AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int> &labels,
                 parser::LinkDirection direction, size_t extend_id, bool is_varlen);

    void PrintGraph();

    void RebuildGraphByHops(std::vector<Graph> &queryGraphs);

    void CopyGraph(Graph &new_graph, const Graph *graph, bool withEdge);
};

};  // namespace rewrite
};  // namespace cypher
