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

#include "cypher/execution_plan/optimization/rewrite/schema_rewrite.h"
#include <utility>

// #define DEBUG
#define LOG(x) std::cout << x << std::endl;

namespace cypher::rewrite {

SchemaRewrite::SchemaRewrite(
    std::map<std::string, int>* label2idx, std::vector<std::string>* idx2label,
    std::map<size_t, cypher::NodeID>* vidx2pidx, std::map<cypher::NodeID, size_t>* pidx2vidx,
    std::map<size_t, cypher::NodeID>* eidx2pidx, cypher::SchemaNodeMap* m_schema_node_map,
    cypher::SchemaRelpMap* m_schema_relp_map, std::vector<PathInfo>* path_infos)
    : label2idx(label2idx),
      idx2label(idx2label),
      vidx2pidx(vidx2pidx),
      pidx2vidx(pidx2vidx),
      eidx2pidx(eidx2pidx),
      m_schema_node_map(m_schema_node_map),
      m_schema_relp_map(m_schema_relp_map),
      m_path_infos(path_infos) {}

// 根据schema信息和cypher信息获取有效路径
std::vector<cypher::SchemaGraphMap> SchemaRewrite::GetEffectivePath(Graph* target_graph,
                                                                    Graph* query_graph) {
    // 初始化相关参数
    this->target_graph = target_graph;
    this->query_graph = query_graph;

    target_size = target_graph->m_nodes.size();
    query_size = query_graph->m_nodes.size();
    size_t query_edge_size = query_graph->m_edges.size();
    edge_core.resize(query_edge_size);
    edge_stateinfo_core.resize(query_edge_size);
    m_direction_core.resize(query_edge_size);
    visited.resize(query_size);
    edge_visited.resize(target_graph->m_edges.size());
    core_2.resize(query_size);

#ifdef DEBUG
    PrintLabel2Idx();
    Printidx2pidx();
    std::cout << "目标图:" << std::endl;
    target_graph->PrintGraph();
    std::cout << "查询图:" << std::endl;
    query_graph->PrintGraph();
#endif

    // 回溯
    for (size_t i = 0; i < target_size; i++) {
        Reset();
        if (CheckNodeLabel(0, i)) {
            core_2[0] = i;
            depth++;
            MatchRecursive(0, i);
        }
    }

    // #ifdef DEBUG
    //     size_t idx=0;
    //     for(auto sgm:sgm){
    //         std::cout<<"SchemaGraphMap in schema rewrite"<<idx<<":"<<std::endl;
    //         for(auto it:sgm.first){
    //             std::cout<<"id:"<<it.first<<",label:"<<it.second<<std::endl;
    //         }
    //         for(auto it:sgm.second){
    //             std::string label_str;
    //             for(auto str:std::get<2>(it.second)){
    //                 label_str+=str;
    //                 label_str+=".";
    //             }
    //             std::cout<<"id:"<<it.first<<",label:"<<label_str<<std::endl;
    //         }
    //         idx++;
    //     }
    // #endif
    return sgm;
}

void SchemaRewrite::GetEffectivePath() {
    for (size_t i = 0; i < target_size; i++) {
        Reset();
        if (CheckNodeLabel(0, i)) {
            core_2[0] = i;
            depth++;
            MatchRecursive(0, i);
        }
    }
}

// 重置所有状态
void SchemaRewrite::Reset() {
    for (size_t i = 0; i < query_size; i++) {
        core_2[i] = -1;
        visited[i] = false;
    }
    map_cnt = 0;
    depth = 0;
}

void SchemaRewrite::MatchRecursive(size_t vid, size_t t_vid) {
    if (depth == query_size) {
#ifdef DEBUG
        // PrintMapping();
        PrintVarLenMapping();
#endif
        AddVarLenMapping();
        // AddMapping();
    } else {
        std::vector<StateInfo> candidate_state_infos;
        GenCandidateStateInfo(candidate_state_infos);

        // #ifdef DEBUG
        //     std::cout<<"num of
        //     stateinfos:"<<candidate_state_infos.size()<<",depth:"<<depth<<std::endl;
        //     for(StateInfo si:candidate_state_infos){
        //         std::cout<<"vid:"<<si.m_vid<<std::endl;
        //         std::cout<<"next vid:"<<si.m_next_vid<<std::endl;
        //         std::cout<<"eid:"<<si.m_eid<<std::endl;
        //     }
        // #endif

        for (StateInfo& si : candidate_state_infos) {
            for (auto it = si.m_id_map.begin(); it != si.m_id_map.end(); it++) {
#ifdef DEBUG
                std::cout << "check:query:" << si.m_next_vid << ",target:" << it->first
                          << std::endl;
#endif
                if (CheckNodeLabel(si.m_next_vid, it->first)) {
                    core_2[si.m_next_vid] = it->first;
                    depth++;
                    edge_core[si.m_eid] = si.m_id_map[it->first];
                    edge_stateinfo_core[si.m_eid] = si;
                    MatchRecursive(si.m_next_vid, it->first);
                    core_2[si.m_next_vid] = -1;
                    depth--;
                }
            }
        }
    }
}

void SchemaRewrite::GenCandidateStateInfo(std::vector<StateInfo>& candidate_state_infos) {
    for (size_t i = 0; i < query_size; i++) {
        if (core_2[i] > -1) {
            Node node = query_graph->m_nodes[i];
            for (size_t eid : node.m_outedges) {
                Edge& edge = query_graph->m_edges[eid];
                size_t target_id = edge.GetTarId();
                if (core_2[target_id] == -1) {
                    std::set<EidDirection> eid_directions;
                    GetNextEdgeIdDirections(eid_directions, &edge, core_2[i], 0);

                    std::map<size_t, std::set<EidDirection>> id_map;
                    GetNextTVidByEIdDirection(id_map, eid_directions, core_2[i]);
                    // StateInfo si(i,edge.m_target_id,eid,0,&id_map);
                    // candidate_state_infos.push_back(si);
                    candidate_state_infos.emplace_back(StateInfo(
                        i, target_id, eid, parser::LinkDirection::LEFT_TO_RIGHT, &id_map));
                }
            }
            if (!candidate_state_infos.empty()) {
                return;
            }
            for (size_t eid : node.m_inedges) {
                Edge edge = query_graph->m_edges[eid];
                size_t source_id = edge.GetSrcId();
                if (core_2[source_id] == -1) {
                    std::set<EidDirection> eid_directions;
                    GetNextEdgeIdDirections(eid_directions, &edge, core_2[i], 1);

                    std::map<size_t, std::set<EidDirection>> id_map;
                    GetNextTVidByEIdDirection(id_map, eid_directions, core_2[i]);
                    // StateInfo si(i,edge.m_source_id,eid,1,&id_map);
                    candidate_state_infos.emplace_back(StateInfo(
                        i, source_id, eid, parser::LinkDirection::RIGHT_TO_LEFT, &id_map));
                }
            }
            if (!candidate_state_infos.empty()) {
                return;
            }
            for (size_t eid : node.m_undirectededges) {
                Edge edge = query_graph->m_edges[eid];
                size_t next_vid = edge.m_source_id == i ? edge.m_target_id : edge.m_source_id;
                if (core_2[next_vid] == -1) {
                    std::set<EidDirection> eid_directions;
                    GetNextEdgeIdDirections(eid_directions, &edge, core_2[i], 0);

                    std::map<size_t, std::set<EidDirection>> id_map;
                    GetNextTVidByEIdDirection(id_map, eid_directions, core_2[i]);
                    candidate_state_infos.emplace_back(
                        StateInfo(i, next_vid, eid, parser::LinkDirection::LEFT_TO_RIGHT, &id_map));

                    eid_directions.clear();
                    GetNextEdgeIdDirections(eid_directions, &edge, core_2[i], 1);
                    id_map.clear();
                    GetNextTVidByEIdDirection(id_map, eid_directions, core_2[i]);
                    // StateInfo si(i,next_vid,eid,2,&id_map);
                    candidate_state_infos.emplace_back(
                        StateInfo(i, next_vid, eid, parser::LinkDirection::RIGHT_TO_LEFT, &id_map));
                }
            }
            if (!candidate_state_infos.empty()) {
                return;
            }
        }
    }
    return;
}

// //回溯
// void SchemaRewrite::Backtrack(size_t vid){
//     map_cnt--;
//     visited[vid]=false;
//     core_2[vid]=-1;
// }

// 检查目标图和查询图上当前点的label是否一致
bool SchemaRewrite::CheckNodeLabel(size_t vid, size_t t_vid) {
    Node* query_node = &query_graph->m_nodes[vid];
    Node* target_node = &target_graph->m_nodes[t_vid];
    if (query_node->m_label == -2 || query_node->m_label == target_node->m_label) {
        return true;
    }
    return false;
}
// 根据目标图上的边id集合获取点id集合
void SchemaRewrite::GetNextTVidByEIdDirection(std::map<size_t, std::set<EidDirection>>& id_map,
                                              std::set<EidDirection>& edge_ids, size_t t_vid) {
    for (auto mit = edge_ids.begin(); mit != edge_ids.end(); mit++) {
        Edge& edge = target_graph->m_edges[mit->m_eid];
        size_t next_tvid = edge.m_target_id;
        if (next_tvid == t_vid) {
            next_tvid = edge.m_source_id;
        }
        auto sit = id_map.find(next_tvid);
        if (sit != id_map.end()) {
            sit->second.insert(*mit);
        } else {
            std::set<EidDirection> ids;
            ids.insert(*mit);
            id_map[next_tvid] = ids;
        }
    }
    return;
}

// 根据查询图上id为t_vid的点获取目标图上可行的边id集合,并加入到eid_directions中
void SchemaRewrite::GetNextEdgeIdDirections(std::set<EidDirection>& eid_directions,
                                            Edge* query_edge, size_t t_vid, size_t direction) {
    Node& target_node = target_graph->m_nodes[t_vid];
    if (direction == 0) {
        for (size_t out_edge_id : target_node.m_outedges) {
            Edge& out_edge = target_graph->m_edges[out_edge_id];
            auto out_edge_label = out_edge.m_labels.begin();
            auto it = query_edge->m_labels.find(*out_edge_label);
            if (query_edge->m_labels.size() == 0 || it != query_edge->m_labels.end()) {
                eid_directions.insert(
                    EidDirection(out_edge_id, parser::LinkDirection::LEFT_TO_RIGHT));
            }
        }
    } else if (direction == 1) {
        for (size_t in_edge_id : target_node.m_inedges) {
            Edge& in_edge = target_graph->m_edges[in_edge_id];
            auto in_edge_label = in_edge.m_labels.begin();
            auto it = query_edge->m_labels.find(*in_edge_label);
            if (query_edge->m_labels.size() == 0 || it != query_edge->m_labels.end()) {
                eid_directions.insert(
                    EidDirection(in_edge_id, parser::LinkDirection::RIGHT_TO_LEFT));
            }
        }
    } else if (direction == 2) {
        for (size_t out_edge_id : target_node.m_outedges) {
            Edge& out_edge = target_graph->m_edges[out_edge_id];
            auto out_edge_label = out_edge.m_labels.begin();
            auto it = query_edge->m_labels.find(*out_edge_label);
            if (query_edge->m_labels.size() == 0 || it != query_edge->m_labels.end()) {
                eid_directions.insert(
                    EidDirection(out_edge_id, parser::LinkDirection::LEFT_TO_RIGHT));
            }
        }
        for (size_t in_edge_id : target_node.m_inedges) {
            Edge& in_edge = target_graph->m_edges[in_edge_id];
            auto in_edge_label = in_edge.m_labels.begin();
            auto it = query_edge->m_labels.find(*in_edge_label);
            if (query_edge->m_labels.size() == 0 || it != query_edge->m_labels.end()) {
                eid_directions.insert(
                    EidDirection(in_edge_id, parser::LinkDirection::RIGHT_TO_LEFT));
            }
        }
    }
    return;
}
// //根据label获取其唯一的id
// int SchemaRewrite::GetLabelNum(const std::string& label){
//         auto it=label2idx->find(label);
//         int label_num=-1;
//         if(it==label2idx->end()){
//             label2idx->insert({label,label_cnt});
//             label_num=label_cnt;
//             label_cnt++;
//         }else{
//             label_num=it->second;
//         }
//         return label_num;
// }
// 将匹配的路径加入到schema graph map中
// void SchemaRewrite::AddMapping(){
//     cypher::SchemaNodeMap snm(*m_schema_node_map);
//     cypher::SchemaRelpMap srm(*m_schema_relp_map);
//     //对查询图中每个顶点找到映射的标签
//     for(size_t j=0;j<query_size;j++){
//         int core_id=core_2[j];
//         std::string label=(*idx2label)[core_id];
//         auto p_id_it=vidx2pidx->find(j);
//         auto p_id =p_id_it->second;
//         #ifdef DEBUG
//             std::cout<<"pattern node id:"<<p_id<<",label:"<<label<<std::endl;
//         #endif
//         snm[p_id]=label;
//     }
//     for(Edge& e:query_graph->m_edges){
//         size_t src_id=e.m_source_id,tar_id=e.m_target_id;
//         size_t core_src_id=core_2[src_id];
//         size_t core_tar_id=core_2[tar_id];

//         auto p_id_it=eidx2pidx->find(e.m_id);
//         auto p_id=p_id_it->second;
//         auto srm_it=srm.find(p_id);

//         parser::LinkDirection direction=std::get<3>(srm_it->second);
//         std::set<size_t> edge_ids=edge_core[e.m_id];
//         std::set<std::string> edge_labels;

//         for(auto it=edge_ids.begin();it!=edge_ids.end();it++){
//             Edge& edge=target_graph->m_edges[*it];
//             auto lit=edge.m_labels.begin();
//             edge_labels.insert((*idx2label)[*lit]);
//         }
//         #ifdef DEBUG
//             std::string labels_str="";
//             for(auto label:edge_labels){
//                 labels_str+=label;
//             }
//             std::cout<<"pattern edge id:"<<p_id<<",label:"<<labels_str<<std::endl;
//         #endif
//         if(srm_it!=srm.end()){
//             auto value=srm_it->second;
//             srm_it->second=std::tuple<cypher::NodeID,cypher::NodeID,std::set<std::string>,parser::LinkDirection,int,int>(std::get<0>(value),std::get<1>(value),edge_labels,std::get<3>(value),std::get<4>(value),std::get<5>(value));
//         }
//     }
//     sgm.push_back(std::make_pair(snm,srm));
// }
// //打印当前匹配的路径
// void SchemaRewrite::PrintMapping(){
//     std::cout<<"Node Mapping:"<<std::endl;
//         for (size_t j = 0; j < query_size; j++)
//         {
//             int core_id=core_2[j];
//             std::string label=(*idx2label)[core_id];
//             std::cout<<"("<<core_id<<"["<<label<<"]-"<<j<<")";
//         }
//         std::cout<<std::endl;
//         std::cout<<"Edge Mapping:"<<std::endl;
//         for(Edge e:query_graph->m_edges){
//             size_t src_id=e.m_source_id,tar_id=e.m_target_id;
//             size_t core_src_id=core_2[src_id],core_tar_id=core_2[tar_id];
//             std::set<size_t> edge_ids=edge_core[e.m_id];
//             std::string label_str="";
//             for(auto i=edge_ids.begin();i!=edge_ids.end();i++){
//                 Edge& edge=target_graph->m_edges[*i];
//                 int label_id=0;
//                 for (auto it = edge.m_labels.begin(); it != edge.m_labels.end(); it++)
//                 {
//                     label_id=*it;
//                     break;
//                 }
//                 label_str+=(*idx2label)[label_id];
//                 label_str+=".";
//             }
//             std::string src_label=(*idx2label)[core_src_id];
//             std::string tar_label=(*idx2label)[core_tar_id];
//             std::cout<<"("<<src_id<<"["<<src_label<<"])-["<<label_str<<"]-("<<tar_id<<"["<<tar_label<<"])
//             "<<std::endl;
//         }
// }

// 将匹配的路径加入到schema graph map中
void SchemaRewrite::AddVarLenMapping() {
    cypher::SchemaNodeMap snm(*m_schema_node_map);
    cypher::SchemaRelpMap srm(*m_schema_relp_map);
    for (size_t j = 0; j < original_graph->m_nodes.size(); j++) {
        int core_id = core_2[j];
        std::string& label = (*idx2label)[core_id];
        auto p_id_it = vidx2pidx->find(j);
        auto p_id = p_id_it->second;
#ifdef DEBUG
        std::cout << "pattern node id:" << p_id << ",label:" << label << std::endl;
#endif
        snm[p_id] = label;
    }

    Path* path = nullptr;
    int last_extend_id = -1;
    std::vector<std::pair<cypher::RelpID, Path*>> path_map;
    bool is_start = false;

    for (Edge& e : query_graph->m_edges) {
        if (!e.m_is_varlen) {
            // 不是扩展边，直接添加到map中
            //  size_t src_id=e.m_source_id,tar_id=e.m_target_id;
            //  size_t core_src_id=core_2[src_id];
            //  size_t core_tar_id=core_2[tar_id];

#ifdef DEBUG
            std::cout << "query graph edge id:" << e.m_id << std::endl;
#endif
            auto p_id_it = eidx2pidx->find(e.m_extend_id);
            auto p_id = p_id_it->second;
            auto srm_it = srm.find(p_id);

            parser::LinkDirection direction = std::get<3>(srm_it->second);
            std::set<EidDirection>& edge_ids = edge_core[e.m_id];
            std::set<std::string> edge_labels;

            for (auto it = edge_ids.begin(); it != edge_ids.end(); it++) {
                Edge& edge = target_graph->m_edges[it->m_eid];
                auto lit = edge.m_labels.begin();
                edge_labels.insert((*idx2label)[*lit]);
            }
#ifdef DEBUG
            std::string labels_str = "";
            for (auto label : edge_labels) {
                labels_str += label;
                labels_str += ".";
            }
            std::cout << "no var len,pattern edge id:" << p_id << ",label:" << labels_str
                      << std::endl;
#endif
            if (srm_it != srm.end()) {
                auto& value = srm_it->second;

                // 加入方向
                StateInfo& si = edge_stateinfo_core[e.m_id];
                if (direction == parser::LinkDirection::DIR_NOT_SPECIFIED) {
                    direction = si.m_direction;
                    if ((int)si.m_vid != std::get<0>(value)) {
                        if (direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                            direction = parser::LinkDirection::RIGHT_TO_LEFT;
                        } else {
                            direction = parser::LinkDirection::LEFT_TO_RIGHT;
                        }
                    }
                }

                srm_it->second = std::tuple<cypher::NodeID, cypher::NodeID, std::set<std::string>,
                                            parser::LinkDirection, int, int>(
                    std::get<0>(value), std::get<1>(value), edge_labels, direction,
                    std::get<4>(value), std::get<5>(value));
            }
        } else {
            // 是扩展边，需要继续连接path
            if (e.m_extend_id != last_extend_id) {
                last_extend_id = e.m_extend_id;
                // std::cout<<"Extend Edge id "<<e.m_extend_id<<" mapping"<<std::endl;
                path = new Path();
                auto p_id_it = eidx2pidx->find(e.m_extend_id);
                auto p_id = p_id_it->second;
                path_map.emplace_back(std::pair<cypher::RelpID, Path*>(p_id, path));
                is_start = true;
#ifdef DEBUG
                std::cout << "p id :" << p_id << std::endl;
                std::cout << "path :" << path->ToStringV1() << std::endl;
#endif
            }
            std::set<EidDirection>& edge_ids = edge_core[e.m_id];
            std::set<std::string> edge_labels;
            for (auto i = edge_ids.begin(); i != edge_ids.end(); i++) {
                Edge& edge = target_graph->m_edges[i->m_eid];
                int label_id = 0;
                for (auto it = edge.m_labels.begin(); it != edge.m_labels.end(); it++) {
                    label_id = *it;
                    break;
                }
                edge_labels.insert((*idx2label)[label_id]);
#ifdef DEBUG
                for (auto elabel : edge_labels) {
                    std::cout << "edge_label:" << elabel << std::endl;
                }
#endif
            }
            size_t src_id = e.m_source_id, tar_id = e.m_target_id;
            size_t core_src_id = core_2[src_id], core_tar_id = core_2[tar_id];
            std::string& src_label = (*idx2label)[core_src_id];
            std::string& tar_label = (*idx2label)[core_tar_id];

            // 加入方向
            StateInfo si = edge_stateinfo_core[e.m_id];
            parser::LinkDirection temp_direction = si.m_direction;
            if (si.m_vid != src_id) {
                if (temp_direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                    temp_direction = parser::LinkDirection::RIGHT_TO_LEFT;
                } else if (temp_direction == parser::LinkDirection::RIGHT_TO_LEFT) {
                    temp_direction = parser::LinkDirection::LEFT_TO_RIGHT;
                }
            }

            if (is_start) {
                path->append(edge_labels, src_label, tar_label, temp_direction);
                is_start = false;
            } else {
                path->append(edge_labels, tar_label, temp_direction);
            }
#ifdef DEBUG
            for (auto elabel : path->m_elabels[0]) {
                std::cout << "edge_label:" << elabel << std::endl;
            }
            LOG(std::string("PATHS:") + path->ToStringV1())
#endif
        }
    }

#ifdef DEBUG
    LOG("DEBUG RES PATHS:！")
    for (auto pair : path_map) {
        // m_path_info->AddPath(pair.first,pair.second);
        std::cout << "relp id:" << pair.first << " path:" << pair.second->ToStringV1() << std::endl;
    }
#endif

    // if(m_path_info==nullptr || m_path_info->CheckNewPathInfo(&snm,&srm)){
    //     // m_path_info=new PathInfo(&snm,&srm);
    //     m_path_infos->push_back(PathInfo(&snm,&srm));
    //     m_path_info=&m_path_infos->back();
    //     for(auto pair:path_map){
    //         m_path_info->AddPath(pair.first,pair.second);
    //     }
    // }else{
    //     //不需要更新path_info，直接add path
    //     for(auto pair:path_map){
    //         m_path_info->AddPath(pair.first,pair.second);
    //     }
    // }
    int path_info_id = GetPathInfoId(&snm, &srm);
    if (path_info_id == -1) {
        // 未匹配到之前任意一个path_info，新建path_info
        m_path_infos->emplace_back(PathInfo(&snm, &srm));
        m_path_info = &m_path_infos->back();

        for (auto& pair : path_map) {
            m_path_info->AddPath(pair.first, pair.second);
        }
    } else {
        m_path_info = &m_path_infos->at(path_info_id);
        for (auto& pair : path_map) {
            m_path_info->AddPath(pair.first, pair.second);
        }
    }
}

// 打印当前匹配的路径,包含可变长的路径
void SchemaRewrite::PrintVarLenMapping() {
    std::cout << "Node Mapping:" << std::endl;
    for (size_t j = 0; j < query_size; j++) {
        int core_id = core_2[j];
        std::string label = (*idx2label)[core_id];
        std::cout << "(" << core_id << "[" << label << "]-" << j << ")";
    }
    std::cout << std::endl;

    int last_extend_id = -1;
    std::vector<Path> res;
    Edge* original_edge = nullptr;
    Path* path = nullptr;
    bool is_start = false;

    std::cout << "Edge Mapping:" << std::endl;
    for (Edge e : query_graph->m_edges) {
        if (!e.m_is_varlen) {
            // 不是扩展边，直接打印
            size_t src_id = e.m_source_id, tar_id = e.m_target_id;
            size_t core_src_id = core_2[src_id], core_tar_id = core_2[tar_id];
            std::set<EidDirection> edge_ids = edge_core[e.m_id];
            std::string label_str = "";

            // 加入方向
            StateInfo si = edge_stateinfo_core[e.m_id];
            parser::LinkDirection temp_direction = si.m_direction;
            if (si.m_vid != src_id) {
                if (temp_direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                    temp_direction = parser::LinkDirection::RIGHT_TO_LEFT;
                } else {
                    temp_direction = parser::LinkDirection::LEFT_TO_RIGHT;
                }
            }
            si.Print();

            for (auto i = edge_ids.begin(); i != edge_ids.end(); i++) {
                Edge& edge = target_graph->m_edges[i->m_eid];
                int label_id = 0;
                for (auto it = edge.m_labels.begin(); it != edge.m_labels.end(); it++) {
                    label_id = *it;
                    break;
                }
                label_str += (*idx2label)[label_id];
                label_str += ".";
            }
            std::string src_label = (*idx2label)[core_src_id];
            std::string tar_label = (*idx2label)[core_tar_id];

            if (temp_direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                std::cout << "(" << src_id << "[" << src_label << "])-[" << label_str << "]->("
                          << tar_id << "[" << tar_label << "]) " << std::endl;
            } else if (temp_direction == parser::LinkDirection::RIGHT_TO_LEFT) {
                std::cout << "(" << src_id << "[" << src_label << "])<-[" << label_str << "]-("
                          << tar_id << "[" << tar_label << "]) " << std::endl;
            } else {
                std::cout << "(" << src_id << "[" << src_label << "])-[" << label_str << "]-("
                          << tar_id << "[" << tar_label << "]) " << std::endl;
            }

        } else {
            // 是扩展边，需要继续连接path
            if (e.m_extend_id != last_extend_id) {
                last_extend_id = e.m_extend_id;

                // std::cout<<"Extend Edge id "<<e.m_id<<" mapping"<<std::endl;
                original_edge = &original_graph->m_edges[e.m_extend_id];
                path = new Path();
                original_edge->m_paths.push_back(path);
                is_start = true;
            }
            size_t src_id = e.m_source_id, tar_id = e.m_target_id;
            size_t core_src_id = core_2[src_id], core_tar_id = core_2[tar_id];
            std::set<EidDirection> edge_ids = edge_core[e.m_id];

            // 加入方向
            StateInfo si = edge_stateinfo_core[e.m_id];
            parser::LinkDirection temp_direction = si.m_direction;
            if (si.m_vid != src_id) {
                if (temp_direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                    temp_direction = parser::LinkDirection::RIGHT_TO_LEFT;
                } else if (temp_direction == parser::LinkDirection::RIGHT_TO_LEFT) {
                    temp_direction = parser::LinkDirection::LEFT_TO_RIGHT;
                }
            }
            // si.Print();

            std::string label_str = "";
            for (auto i = edge_ids.begin(); i != edge_ids.end(); i++) {
                Edge& edge = target_graph->m_edges[i->m_eid];
                int label_id = 0;
                for (auto it = edge.m_labels.begin(); it != edge.m_labels.end(); it++) {
                    label_id = *it;
                    break;
                }
                label_str += (*idx2label)[label_id];
                label_str += ".";
            }

            std::string src_label = (*idx2label)[core_src_id];
            std::string tar_label = (*idx2label)[core_tar_id];
            if (temp_direction == parser::LinkDirection::LEFT_TO_RIGHT) {
                std::cout << "(" << src_id << "[" << src_label << "])-[" << label_str << "]->("
                          << tar_id << "[" << tar_label << "]) " << std::endl;
            } else if (temp_direction == parser::LinkDirection::RIGHT_TO_LEFT) {
                std::cout << "(" << src_id << "[" << src_label << "])<-[" << label_str << "]-("
                          << tar_id << "[" << tar_label << "]) " << std::endl;
            } else {
                std::cout << "(" << src_id << "[" << src_label << "])-[" << label_str << "]-("
                          << tar_id << "[" << tar_label << "]) " << std::endl;
            }

            if (is_start) {
                path->append(label_str, src_label, tar_label, e.m_direction);
                is_start = false;
            } else {
                path->append(label_str, tar_label, e.m_direction);
            }
        }
    }
}

void SchemaRewrite::PrintLabel2Idx() {
    for (auto it : *label2idx) {
        std::cout << it.first << " " << it.second << std::endl;
    }
}

void SchemaRewrite::Printidx2pidx() {
    LOG("vid2pidx:");
    for (auto it : *vidx2pidx) {
        std::cout << "vid:" << it.first << ",pvid:" << it.second << std::endl;
    }
    LOG("eid2pidx:");
    for (auto it : *eidx2pidx) {
        std::cout << "eid:" << it.first << ",pvid:" << it.second << std::endl;
    }
}

int SchemaRewrite::GetPathInfoId(cypher::SchemaNodeMap* snm, cypher::SchemaRelpMap* srm) {
    for (size_t i = 0; i < m_path_infos->size(); i++) {
        auto& path_info = (*m_path_infos)[i];
        if (!path_info.CheckNewPathInfo(snm, srm)) {
            // 该map info符合schema node map 和 schema relp map，直接返回id
            return i;
        }
    }
    return -1;
}

};  // namespace cypher::rewrite
