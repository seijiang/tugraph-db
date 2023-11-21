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

#include "cypher/execution_plan/optimization/rewrite/path_info.h"

namespace cypher::rewrite {
PathInfo::PathInfo(cypher::SchemaNodeMap* schema_node_map, cypher::SchemaRelpMap* schema_relp_map)
    : m_schema_node_map(*schema_node_map), m_schema_relp_map(*schema_relp_map) {}

// //若原始查询图的顶点标签相同、非可变长边的标签集合相等，则视为PathInfo相等
// bool PathInfo::operator==(const PathInfo& other){
//     // 检查原始查询图的顶点标签是否相同
//     auto o_it=other.m_schema_node_map.begin();
//     for(auto it=m_schema_node_map.begin();it!=m_schema_node_map.end();it++,o_it++){
//         auto& label=it->second;
//         auto& o_label=o_it->second;
//         if(label!=o_label){
//             return false;
//         }
//     }
//     // 检查非可变长边的标签集合
//     auto oe_it=other.m_schema_relp_map.begin();
//     for(auto it=m_schema_relp_map.begin();it!=m_schema_relp_map.end();it++,oe_it++){
//         auto& labels=std::get<2>(it->second);
//         auto& o_labels=std::get<2>(oe_it->second);
//         if(labels!=o_labels){
//             return false;
//         }
//     }
//     return true;
// };

void PathInfo::AddPath(cypher::RelpID& rid, Path*& path) {
    auto it = m_var_len_paths_map.find(rid);
    if (it == m_var_len_paths_map.end()) {
        // std::vector<Path> paths={*path};
        // // paths.push_back(*path);
        // m_var_len_paths_map[rid]=std::move(paths);
        m_var_len_paths_map[rid] = std::vector<Path>{*path};
    } else {
        if (std::find(it->second.begin(), it->second.end(), *path) == it->second.end())
            it->second.push_back(*path);
    }
}

// 检查是否需要创建一个新的map info，当节点标签都相同，非可变长边的标签、方向相同时，则不需要新建。
bool PathInfo::CheckNewPathInfo(cypher::SchemaNodeMap* snm, cypher::SchemaRelpMap* srm) {
    // 检查原始查询图的顶点标签是否相同
    auto o_it = snm->begin();
    for (auto it = m_schema_node_map.begin(); it != m_schema_node_map.end(); it++, o_it++) {
        auto& label = it->second;
        auto& o_label = o_it->second;
        if (label != o_label) {
            // std::cout<<label<<"!="<<o_label<<std::endl;
            return true;
        }
    }

    // 检查非可变长边的标签集合
    auto oe_it = srm->begin();
    for (auto it = m_schema_relp_map.begin(); it != m_schema_relp_map.end(); it++, oe_it++) {
        int min_hop = std::get<4>(it->second);
        int max_hop = std::get<5>(it->second);
        if (min_hop != -1 && max_hop != -1) {
            // 可变长边，直接跳过
            continue;
        }
        auto& labels = std::get<2>(it->second);
        auto& o_labels = std::get<2>(oe_it->second);
        if (labels != o_labels) {
            // std::cout<<"labels"<<"!="<<"o_labels"<<std::endl;
            return true;
        }
        // 检查非可变长边方向
        auto& direction = std::get<3>(it->second);
        auto& o_direction = std::get<3>(oe_it->second);
        if (direction != o_direction) {
            return true;
        }
    }

    // std::cout<<"do not need new map info!"<<std::endl;
    return false;
}

std::string PathInfo::ToString() {
    std::string res;
    for (auto node : m_schema_node_map) {
        res.append("node " + std::to_string(+node.first) + "(" + node.second + ")\n");
    }
    for (auto relp : m_schema_relp_map) {
        std::string relp_s = "relp " + std::to_string(relp.first) + "(";
        for (auto label : std::get<2>(relp.second)) {
            relp_s.append(label + ",");
        }
        if (std::get<2>(relp.second).size() > 0) relp_s.pop_back();
        relp_s.append(")");
        res.append(relp_s + "\n");
    }
    for (auto varlen_relp : m_var_len_paths_map) {
        std::string varlen_relp_s = "varlen relp " + std::to_string(varlen_relp.first) + "{\n";

        for (auto path : varlen_relp.second) {
            varlen_relp_s.append("\t" + path.ToStringV1() + "\n");
        }
        varlen_relp_s.append("}");
        res.append(varlen_relp_s + "\n");
    }
    return res;
    // for(auto varlen_relp:m_var_len_paths_map){

    // }
}
};  // namespace cypher::rewrite
