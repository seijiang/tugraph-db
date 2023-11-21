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

#include "cypher/execution_plan/optimization/rewrite/graph.h"

// #define DEBUG

namespace cypher {
namespace rewrite {
void Graph::AddNode(size_t id, int label_num) { m_nodes.emplace_back(Node(id, label_num)); }

void Graph::AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int>& labels,
                    parser::LinkDirection direction) {
    m_edges.emplace_back(Edge(id, source_id, target_id, labels, direction));
    if (direction == parser::LinkDirection::LEFT_TO_RIGHT) {
        m_nodes[source_id].m_outedges.emplace_back(id);
        m_nodes[target_id].m_inedges.emplace_back(id);
    } else if (direction == parser::LinkDirection::RIGHT_TO_LEFT) {
        m_nodes[source_id].m_inedges.emplace_back(id);
        m_nodes[target_id].m_outedges.emplace_back(id);
    } else {
        m_nodes[source_id].m_undirectededges.emplace_back(id);
        m_nodes[target_id].m_undirectededges.emplace_back(id);
    }
}
void Graph::AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int>& labels,
                    parser::LinkDirection direction, int min_hop, int max_hop) {
    m_edges.emplace_back(Edge(id, source_id, target_id, labels, direction, min_hop, max_hop));
    if (direction == parser::LinkDirection::LEFT_TO_RIGHT) {
        m_nodes[source_id].m_outedges.emplace_back(id);
        m_nodes[target_id].m_inedges.emplace_back(id);
    } else if (direction == parser::LinkDirection::RIGHT_TO_LEFT) {
        m_nodes[source_id].m_inedges.emplace_back(id);
        m_nodes[target_id].m_outedges.emplace_back(id);
    } else {
        m_nodes[source_id].m_undirectededges.emplace_back(id);
        m_nodes[target_id].m_undirectededges.emplace_back(id);
    }
}

void Graph::AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int>& labels,
                    parser::LinkDirection direction, size_t extend_id) {
    this->AddEdge(id, source_id, target_id, labels, direction);
    this->m_edges.back().m_extend_id = extend_id;
}

void Graph::AddEdge(size_t id, size_t source_id, size_t target_id, const std::set<int>& labels,
                    parser::LinkDirection direction, size_t extend_id, bool is_varlen) {
    this->AddEdge(id, source_id, target_id, labels, direction);
    this->m_edges.back().m_extend_id = extend_id;
    this->m_edges.back().m_is_varlen = is_varlen;
#ifdef DEBUG
    std::cout << "edge id:" << id << ",extend id:" << this->m_edges.back().m_extend_id
              << ",is_varlen:" << this->m_edges.back().m_is_varlen << std::endl;
#endif
}

void Graph::PrintGraph() {
    for (Node node : m_nodes) {
        std::cout << "Node id:" << node.m_id << std::endl;
        std::cout << "Label id:" << node.m_label << std::endl;
        std::cout << "In edges:" << std::endl;
        for (size_t eid : node.m_inedges) {
            Edge& edge = m_edges[eid];
            // std::cout<<"eid:"<<edge.m_id<<" - src id:"<<edge.m_source_id<<".";
            std::cout << "eid:" << edge.m_id << "-src id:" << edge.m_source_id
                      << "-tar id:" << edge.m_target_id << "-direction:" << edge.m_direction << ".";
        }
        std::cout << std::endl;

        std::cout << "Out edges:" << std::endl;
        for (size_t eid : node.m_outedges) {
            Edge& edge = m_edges[eid];
            // std::cout<<"eid:"<<edge.m_id<<" - tar id:"<<edge.m_target_id<<".";
            std::cout << "eid:" << edge.m_id << "-src id:" << edge.m_source_id
                      << "-tar id:" << edge.m_target_id << "-direction:" << edge.m_direction << ".";
        }
        std::cout << std::endl;

        std::cout << "Undirection edges:" << std::endl;
        for (size_t eid : node.m_undirectededges) {
            Edge& edge = m_edges[eid];
            // std::cout<<edge.m_id<<"-"<<edge.m_direction<<".";
            std::cout << "eid:" << edge.m_id << " - src id:" << edge.m_source_id
                      << " - tar id:" << edge.m_target_id << ".";
        }
        std::cout << std::endl;
    }
    for (Edge edge : m_edges) {
        std::cout << "Edge id:" << edge.m_id << std::endl;
        std::cout << "Label id:" << std::endl;
        for (auto it = edge.m_labels.begin(); it != edge.m_labels.end(); it++) {
            std::cout << *it << std::endl;
        }
        std::cout << "src:" << edge.m_source_id << ",tar:" << edge.m_target_id << std::endl;
        std::cout << "direction:" << edge.m_direction << std::endl;
        std::cout << "min hop:" << edge.m_min_hop << ",max hop:" << edge.m_max_hop << std::endl;
        std::cout << "is varlen:" << edge.m_is_varlen << ",extend id:" << edge.m_extend_id
                  << std::endl;
    }
}

void Graph::RebuildGraphByHops(std::vector<Graph>& queryGraphs) {
    Graph graph;
    CopyGraph(graph, this, false);
    queryGraphs.push_back(graph);
    for (Edge& edge : this->m_edges) {
        if (edge.m_min_hop != -1 && edge.m_max_hop != -1) {
            std::vector<Graph> new_queryGraphs;
            for (Graph& temp_graph : queryGraphs) {
                for (int i = edge.m_min_hop; i <= edge.m_max_hop; i++) {
                    Graph res;
                    CopyGraph(res, &temp_graph, true);
                    size_t last_nid = edge.m_source_id;
                    for (int j = 0; j < i - 1; j++) {
                        res.AddNode(res.m_node_cnt, -2);
                        res.AddEdge(res.m_edge_cnt, last_nid, res.m_node_cnt, edge.m_labels,
                                    edge.m_direction, edge.m_id, true);
                        last_nid = res.m_node_cnt;
                        res.m_node_cnt++;
                        res.m_edge_cnt++;
                    }
                    res.AddEdge(res.m_edge_cnt, last_nid, edge.m_target_id, edge.m_labels,
                                edge.m_direction, edge.m_id, true);
                    res.m_edge_cnt++;
                    new_queryGraphs.push_back(res);
                }
            }
            queryGraphs = new_queryGraphs;
        } else {
            for (Graph& temp_graph : queryGraphs) {
                temp_graph.AddEdge(temp_graph.m_edge_cnt, edge.m_source_id, edge.m_target_id,
                                   edge.m_labels, edge.m_direction, edge.m_id, false);
                temp_graph.m_edge_cnt++;
            }
        }
    }
}

void Graph::CopyGraph(Graph& new_graph, const Graph* graph, bool withEdge) {
    for (const Node& node : graph->m_nodes) {
        new_graph.AddNode(node.m_id, node.m_label);
    }
    new_graph.m_node_cnt = graph->m_nodes.size();

    if (withEdge) {
        for (const Edge& edge : graph->m_edges) {
            new_graph.AddEdge(edge.m_id, edge.m_source_id, edge.m_target_id, edge.m_labels,
                              edge.m_direction);
            // 补上扩展id和是否为可变长边
            new_graph.m_edges.back().m_extend_id = edge.m_extend_id;
            new_graph.m_edges.back().m_is_varlen = edge.m_is_varlen;
        }
        new_graph.m_edge_cnt = graph->m_edges.size();
    }
}

};  // namespace rewrite
};  // namespace cypher
