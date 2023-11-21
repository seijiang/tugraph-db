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

#include "cypher/execution_plan/optimization/rewrite/rewrite_helper.h"
#define LOG(x) std::cout << x << std::endl;
// #define DEBUG

namespace cypher::rewrite {

RewriteHelper::RewriteHelper(const lgraph::SchemaInfo& schema_info,
                             cypher::SchemaNodeMap* schema_node_map,
                             cypher::SchemaRelpMap* schema_relp_map) {
    // 根据schema构建目标图
    m_schema_node_map = schema_node_map;
    m_schema_relp_map = schema_relp_map;
    label2idx.insert({"", -2});
    std::vector<std::string> v_labels = schema_info.v_schema_manager.GetAllLabels();
    int i = 0;
    for (; (size_t)i < v_labels.size(); i++) {
        label2idx.insert({v_labels[i], i});
        idx2label.emplace_back(v_labels[i]);
#ifdef DEBUG
        std::cout << "add label: idx:" << i << ",label:" << v_labels[i] << std::endl;
#endif
        target_graph.AddNode((size_t)i, GetLabelNum(v_labels[i]));
    }

    lgraph::SchemaManager e_schema_manager = schema_info.e_schema_manager;
    size_t label_num = 0;
    const lgraph::Schema* schema;
    size_t e_cnt = 0;
    while ((schema = e_schema_manager.GetSchema(label_num))) {
        // std::cout<<"schema label:"<<schema->GetLabel()<<std::endl;
        label2idx.insert({schema->GetLabel(), i});
        idx2label.emplace_back(schema->GetLabel());
        i++;
        const lgraph::EdgeConstraints& ec = schema->GetEdgeConstraints();
        for (auto& pair : ec) {
            // std::cout<<"src:"<<pair.first<<",dst:"<<pair.second<<std::endl;
            std::set<int> label_nums;
            label_nums.insert(GetLabelNum(schema->GetLabel()));
            target_graph.AddEdge(e_cnt, (size_t)GetLabelNum(pair.first),
                                 (size_t)GetLabelNum(pair.second), label_nums,
                                 parser::LinkDirection::LEFT_TO_RIGHT);
// std::cout<<"edge id :"<<e_cnt<<",label num:"<<GetLabelNum(schema->GetLabel())<<",src
// id:"<<(size_t)GetLabelNum(pair.first)<<",dst id:"<<(size_t)GetLabelNum(pair.second)<<std::endl;
#ifdef DEBUG
            std::cout << "edge id :" << e_cnt << ",label:" << schema->GetLabel() << std::endl;
#endif
            e_cnt++;
        }
        label_num++;
    }

    // cypher语句构建查询图
    size_t cnt = 0;
    for (auto it = schema_node_map->begin(); it != schema_node_map->end(); it++) {
        vidx2pidx.insert({cnt, it->first});
        pidx2vidx.insert({it->first, cnt});
        query_graph.AddNode(cnt, GetLabelNum(it->second));
        // std::cout<<"node id :"<<cnt<<",label num:"<<GetLabelNum(it->second)<<std::endl;
        cnt++;
    }
    int var_len_relp_cnt = 0;
    e_cnt = 0;
    for (auto it = schema_relp_map->begin(); it != schema_relp_map->end(); it++) {
        eidx2pidx.insert({e_cnt, it->first});
        auto p_it = pidx2vidx.find(std::get<0>(it->second));
        auto src_idx = p_it->second;
        p_it = pidx2vidx.find(std::get<1>(it->second));
        auto dst_idx = p_it->second;
        std::set<std::string>& labels = std::get<2>(it->second);
        parser::LinkDirection direction = std::get<3>(it->second);
        int min_hop = std::get<4>(it->second);
        int max_hop = std::get<5>(it->second);
        std::set<int> label_nums;
        if (min_hop != -1 && max_hop != -1) {
            need_rebuild = true;
            var_len_relp_cnt += 1;

            // 跳过可变长关系上带标签和最长跳数大于10
            //  if(!labels.empty() || max_hop>10){
            //      skip_rewrite=true;
            //  }

            // 最长跳数大于10
            if (max_hop > 10) {
                skip_rewrite = true;
            }
        }
#ifdef DEBUG
        std::cout << "e_cnt:" << e_cnt << ",var_len_relp_cnt:" << var_len_relp_cnt << std::endl;
#endif

        // //跳过包含0跳、可变长大于1
        // if(min_hop==0 || max_hop==0 || var_len_relp_cnt>1){
        //     skip_rewrite=true;
        //     return;
        // }

        // 跳过包含0跳
        if (min_hop == 0 || max_hop == 0) {
            skip_rewrite = true;
            return;
        }

        if (labels.empty()) {
            query_graph.AddEdge(e_cnt, src_idx, dst_idx, label_nums, direction, min_hop, max_hop);
            // std::cout<<"src:"<<src_idx<<"dst:"<<dst_idx<<"direction:"<<direction<<std::endl;
        } else {
            for (auto lit = labels.begin(); lit != labels.end(); lit++) {
                label_nums.insert(GetLabelNum(*lit));
            }
            query_graph.AddEdge(e_cnt, src_idx, dst_idx, label_nums, direction, min_hop, max_hop);
        }
        // extend_id设置为自己
        query_graph.m_edges.back().m_extend_id = e_cnt;
        e_cnt++;
    }
}

std::vector<PathInfo> RewriteHelper::GetEffectivePath() {
#ifdef DEBUG
    LOG("目标图!");
    target_graph.PrintGraph();
    LOG("属性图!");
    query_graph.PrintGraph();
#endif

    if (skip_rewrite) {
#ifdef DEBUG
        LOG("跳过包含可变长路径优化!");
#endif
        return path_infos;
    }

    // std::vector<Graph> graphs;
    // query_graph.RebuildGraphByHops(graphs);

    if (need_rebuild) {
        std::vector<Graph> graphs;
        query_graph.RebuildGraphByHops(graphs);
#ifdef DEBUG
        LOG("重构图!");
        for (auto graph : graphs) {
            graph.PrintGraph();
            LOG("");
        }
#endif
        for (auto& graph : graphs) {
            SchemaRewrite sr(&label2idx, &idx2label, &vidx2pidx, &pidx2vidx, &eidx2pidx,
                             m_schema_node_map, m_schema_relp_map, &path_infos);
            sr.original_graph = &query_graph;
            sr.GetEffectivePath(&target_graph, &graph);
        }
#ifdef DEBUG
        LOG("可变长路径:");
        // for(auto edge:query_graph.m_edges){
        //     if(edge.m_is_varlen){
        //         std::cout<<"edge id "<<edge.m_id<<" paths:"<<std::endl;
        //         for(auto path:edge.m_paths){
        //             LOG(path->ToString());
        //         }
        //     }
        // }

        auto i = 0;
        for (auto path_info : path_infos) {
            std::cout << "map info " << i << " :" << std::endl;
            i++;
            for (auto it : path_info.m_schema_node_map) {
                std::cout << "id:" << it.first << ",label:" << it.second << std::endl;
            }
            for (auto it : path_info.m_schema_relp_map) {
                std::string label_str;
                for (auto str : std::get<2>(it.second)) {
                    label_str += str;
                    label_str += ".";
                }
                auto src_id = std::get<0>(it.second);
                auto tar_id = std::get<1>(it.second);
                auto direction = std::get<3>(it.second);
                if (direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                    std::cout << "id:" << it.first << ",(" << src_id << ")-[" << label_str << "]->("
                              << tar_id << ") " << std::endl;
                } else if (direction == parser::LinkDirection::RIGHT_TO_LEFT) {
                    std::cout << "id:" << it.first << ",(" << src_id << ")<-[" << label_str << "]-("
                              << tar_id << ") " << std::endl;
                } else {
                    std::cout << "id:" << it.first << ",(" << src_id << ")-[" << label_str << "]-("
                              << tar_id << ") " << std::endl;
                }
            }
            for (auto it : path_info.m_var_len_paths_map) {
                std::cout << "relp id:" << it.first << std::endl;
                for (auto path : it.second) {
                    LOG(path.ToStringV1());
                }
            }
        }
#endif
        return path_infos;
    } else {
        // LOG("have no multi hop!");
        SchemaRewrite sr(&label2idx, &idx2label, &vidx2pidx, &pidx2vidx, &eidx2pidx,
                         m_schema_node_map, m_schema_relp_map, &path_infos);
        sr.original_graph = &query_graph;

        sr.GetEffectivePath(&target_graph, &query_graph);
#ifdef DEBUG
        LOG("非可变长路径：");
        auto i = 0;
        for (auto path_info : path_infos) {
            std::cout << "map info " << i << " :" << std::endl;
            i++;
            for (auto it : path_info.m_schema_node_map) {
                std::cout << "id:" << it.first << ",label:" << it.second << std::endl;
            }
            for (auto it : path_info.m_schema_relp_map) {
                std::string label_str;
                for (auto str : std::get<2>(it.second)) {
                    label_str += str;
                    label_str += ".";
                }
                auto src_id = std::get<0>(it.second);
                auto tar_id = std::get<1>(it.second);
                auto direction = std::get<3>(it.second);
                std::cout << "id:";
                if (direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                    std::cout << "(" << src_id << ")-[" << label_str << "]->(" << tar_id << ") "
                              << std::endl;
                } else if (direction == parser::LinkDirection::RIGHT_TO_LEFT) {
                    std::cout << "(" << src_id << ")<-[" << label_str << "]-(" << tar_id << ") "
                              << std::endl;
                } else {
                    std::cout << "(" << src_id << ")-[" << label_str << "]-(" << tar_id << ") "
                              << std::endl;
                }
            }
            for (auto it : path_info.m_var_len_paths_map) {
                std::cout << "relp id:" << it.first << std::endl;
                for (auto path : it.second) {
                    LOG(path.ToStringV1());
                }
            }
        }
#endif
        return path_infos;
    }
}

// 根据label获取其唯一的id
int RewriteHelper::GetLabelNum(const std::string& label) {
    auto it = label2idx.find(label);
    int label_num = -1;
    if (it == label2idx.end()) {
        label2idx.insert({label, label_cnt});
        label_num = label_cnt;
        label_cnt++;
    } else {
        label_num = it->second;
    }
    return label_num;
}

};  // namespace cypher::rewrite
