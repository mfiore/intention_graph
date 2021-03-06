/* 
 * File:   BayesianNetwork.cpp
 * Author: mfiore
 * 
 * Created on December 15, 2014, 9:47 AM
 */

#include "BayesianNetwork.h"
#include <iostream>

BayesianNetwork::BayesianNetwork(int size, int n_approximation_loops) : approximation_loops(n_approximation_loops) {
    lastIndex = -1;
    bn = new directed_graph<bayes_node>::kernel_1a_c();
    bn->set_number_of_nodes(size);

}

BayesianNetwork::BayesianNetwork(const BayesianNetwork& orig) {
}

BayesianNetwork::~BayesianNetwork() {
}

void BayesianNetwork::updateJoinTree() {
    create_moral_graph(*bn, join_tree);
    create_join_tree(join_tree, join_tree);
}

std::map<NodeAssignment, double> BayesianNetwork::computeProbability(std::map<string, string> evidence) {
    for (int i = 0; i < bn->number_of_nodes(); i++) {
        set_node_as_nonevidence(*bn, i);
        set_node_value(*bn, i, 0);
    }
    for (std::map<string, string>::iterator i = evidence.begin(); i != evidence.end(); i++) {
        set_node_value(*bn, nodeLabels[i->first], nodeValues[i->first][i->second]);
        set_node_as_evidence(*bn, nodeLabels[i->first]);
    }

    std::map<NodeAssignment, double> result;

    if (approximation_loops == 0) {
        bayesian_network_join_tree solution(*bn, join_tree);


        for (std::map<string, std::map<string, int> >::iterator i = nodeValues.begin(); i != nodeValues.end(); i++) {
            // print out the probability distribution for node i.  
            int nodeIndex = nodeLabels[i->first];

            for (int j = 0; j < i->second.size(); j++) {
                //            cout << "p(node " << i->first << "==" << j << ") = " << solution.probability(nodeIndex)(j) << " \n";

                NodeAssignment n;
                n.name = i->first;
                n.value = j;
                result[n] = solution.probability(nodeIndex)(j);
            }

        }
    } else {
        bayesian_network_gibbs_sampler sampler;
        //
        std::vector<int> counts(bn->number_of_nodes(), 0);
        //
        //
        const long rounds = 1500;
        for (long i = 0; i < rounds; i++) {
            sampler.sample_graph(*bn);

            for (int j = 0; j < counts.size(); j++) {
                //            cout<<node_value(*bn,j)<<"\n";
                if (node_value(*bn, j) == 1) {
                    counts[j]++;
                }
            }

        }


        for (std::map<std::string, int>::iterator n = nodeLabels.begin(); n != nodeLabels.end(); n++) {
            NodeAssignment ass;
            ass.name = n->first;
            ass.value = 1;
            //        cout<<counts[n->second]<<"\n";
            result[ass] = (double) counts[n->second] / (double) rounds;

        }
    }


    return result;
}

string BayesianNetwork::getLabel(int i) {
    for (std::map<string, int>::iterator node = nodeLabels.begin(); node != nodeLabels.end(); node++) {
        if (node->second == i) {
            return node->first;
        }
    }
    return "";
}

int BayesianNetwork::getIndex(string label) {
    return nodeLabels[label];
}

std::map<string, int> BayesianNetwork::getValues(int i) {
    for (std::map<string, int>::iterator node = nodeLabels.begin(); node != nodeLabels.end(); node++) {
        if (node->second == i) {

            return nodeValues[node->first];
        }
    }
    return std::map<string, int>();

}

std::map<string, int> BayesianNetwork::getValues(string label) {
    return nodeValues[label];
}

bool BayesianNetwork::addMultiValueNode(string node, string parent, std::vector<string> values) {
    //create new node
    lastIndex = lastIndex + 1;
    int nodeIndex = lastIndex;

    nodeLabels[node] = nodeIndex;
    set_node_num_values(*bn, nodeIndex, values.size());
    numValues.push_back(values.size());

    std::map<string, int> actualValues;
    for (int i = 0; i < values.size(); i++) {
        actualValues[values[i]] = i;
    }
    nodeValues[node] = actualValues;

    bn->add_edge(nodeLabels[parent], nodeIndex);

    std::map<string, int> parentValues = nodeValues[parent];

    int numParts = 0;
    for (int i = 1; i < values.size() + 1; i++) {
        numParts = numParts + i;
    }
    double inc = 1.0 / numParts;

    assignment parent_state;
    parent_state.add(nodeLabels[parent], 1);
    for (int j = 0; j < values.size(); j++) {
        double p = inc * (j + 1);
        set_node_probability(*bn, nodeIndex, j, parent_state, p);

    }

    parent_state.clear();
    parent_state.add(nodeLabels[parent], 0);
    for (int j = 0; j < values.size(); j++) {
        double p = inc * (values.size() - j);
        set_node_probability(*bn, nodeIndex, j, parent_state, p);

    }
    //    cout << "CPT for node " << node << " " << nodeIndex << "= " << node_cpt_filled_out(*bn, nodeIndex) << parent << "\n";
    //    cout << "p(" << nodeIndex << ")=\n";
    conditional_probability_table * table = (&bn->node(nodeIndex).data.table());

    //    for (int i = 0; i < parentValues.size(); i++) {
    //        assignment parent_state;
    //        parent_state.add(nodeLabels[parent], i);
    //        for (int j = 0; j < values.size(); j++) {
    ////            cout << table->probability(j, parent_state) << "\n";
    //
    //        }
    //    }
}

bool BayesianNetwork::addNode(string node, std::vector<string> parents, std::vector<double> influence) {
    //check if parents exist  and load their values in tableVariables;
    std::vector<std::vector<int> > tableVariables;

    BOOST_FOREACH(string parent, parents) {
        if (nodeLabels.find(parent) == nodeLabels.end()) return false;
        std::vector<int> values;
        for (int i = 0; i < nodeValues[parent].size(); i++) {
            values.push_back(i);
        }
        tableVariables.push_back(values);
    }
    //get the matrix corresponding to the parent's combination table
    std::vector<std::vector<int> > tableAssignments;
    if (tableVariables.size() > 0) {
        NestedLoop<int> loop(tableVariables);
        tableAssignments = loop.buildMatrix();
    }
    //create new node
    lastIndex = lastIndex + 1;
    int nodeIndex = lastIndex;

    nodeLabels[node] = nodeIndex;
    set_node_num_values(*bn, nodeIndex, 2);
    numValues.push_back(2);

    std::map<string, int> actualValues;
    actualValues["f"] = 0;
    actualValues["t"] = 1;
    nodeValues[node] = actualValues;


    //add parent edges

    BOOST_FOREACH(string parent, parents) {
        bn->add_edge(nodeLabels[parent], nodeIndex);
    }
    if (parents.size() == 0) {
        double p = 0.5;
        assignment parent_state;
        set_node_probability(*bn, nodeIndex, 1, parent_state, p);
        set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);
    } else {

        for (int i = 0; i < tableAssignments.size(); i++) {
            assignment parent_state;

            double p;
            double max_influence = 0;
            for (int j = 0; j < tableAssignments[i].size(); j++) {
                if (tableAssignments[i][j] == 1) {
                    if (influence[j] > max_influence) {
                        max_influence = influence[j];
                    }
                }
                parent_state.add(nodeLabels[parents[j]], tableAssignments[i][j]);

            }
            p = max_influence != 0 ? max_influence : 0.5;
            set_node_probability(*bn, nodeIndex, 1, parent_state, p);
            set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);
        }
    }
    //        cout << "CPT for node " << node << " " << nodeIndex << " = " << node_cpt_filled_out(*bn, nodeIndex) << "\n";
    //        for (int i = 0; i < parents.size(); i++) {
    //            cout << parents[i] << " ";
    //        }
    //        if (parents.size() > 0) cout << "\n";
    conditional_probability_table * table = (&bn->node(nodeIndex).data.table());

    if (parents.size() > 0) {
        for (int i = 0; i < tableAssignments.size(); i++) {
            assignment parent_state;
            for (int j = 0; j < tableAssignments[i].size(); j++) {
                parent_state.add(nodeLabels[parents[j]], tableAssignments[i][j]);
            }
            //                        cout << table->probability(0, parent_state) << "\n";
            //                        cout << table->probability(1, parent_state) << "\n";
        }
    } else {
        assignment parent_state;
        //                cout << table->probability(0, parent_state) << "\n";
        //                cout << table->probability(1, parent_state) << "\n";
    }
}

//creates a new node with different parents and a choice of  of different probability tables. For now we deal with only binary nodes.
//More complex nodes need to be created by hand

bool BayesianNetwork::addNode(string node, std::vector<string> parents, string probabilityMode, string dominantParent) {
    //check if parents exist  and load their values in tableVariables;
    std::vector<std::vector<int> > tableVariables;

    int dominantParentIndex;

    for (int i = 0; i < parents.size(); i++) {
        if (parents[i] == dominantParent) {
            dominantParentIndex = i;
            break;
        }
    }

    BOOST_FOREACH(string parent, parents) {
        if (nodeLabels.find(parent) == nodeLabels.end()) return false;
        std::vector<int> values;
        for (int i = 0; i < nodeValues[parent].size(); i++) {
            values.push_back(i);
        }
        tableVariables.push_back(values);
    }
    //get the matrix corresponding to the parent's combination table
    std::vector<std::vector<int> > tableAssignments;
    if (tableVariables.size() > 0) {
        NestedLoop<int> loop(tableVariables);
        tableAssignments = loop.buildMatrix();
    }
    //create new node
    lastIndex = lastIndex + 1;
    int nodeIndex = lastIndex;

    nodeLabels[node] = nodeIndex;
    set_node_num_values(*bn, nodeIndex, 2);
    numValues.push_back(2);

    std::map<string, int> actualValues;
    actualValues["f"] = 0;
    actualValues["t"] = 1;
    nodeValues[node] = actualValues;



    //add parent edges

    BOOST_FOREACH(string parent, parents) {
        bn->add_edge(nodeLabels[parent], nodeIndex);
    }


    if (probabilityMode == "uniform") {
        double p = 1.0 / 2;
        if (parents.size() == 0) {
            assignment parent_state;
            set_node_probability(*bn, nodeIndex, 1, parent_state, p);
            set_node_probability(*bn, nodeIndex, 0, parent_state, p);
        } else {
            for (int i = 0; i < tableAssignments.size(); i++) {
                assignment parent_state;
                for (int j = 0; j < tableAssignments[i].size() - 1; j++) {
                    parent_state.add(nodeLabels[parents[j]], tableAssignments[i][j]);
                }
                set_node_probability(*bn, nodeIndex, 1, parent_state, p);
                set_node_probability(*bn, nodeIndex, 0, parent_state, p);

            }
        }
    } else if (probabilityMode == "equalWeight") {
        if (parents.size() == 0) {
            double p = 0.7;
            assignment parent_state;
            set_node_probability(*bn, nodeIndex, 1, parent_state, p);
            set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);
        } else {
            for (int i = 0; i < tableAssignments.size(); i++) {
                assignment parent_state;
                int npos = 0;
                for (int j = 0; j < tableAssignments[i].size(); j++) {
                    parent_state.add(nodeLabels[parents[j]], tableAssignments[i][j]);
                    if (tableAssignments[i][j] == 1) {
                        npos++;
                    }
                }
                double p;
                if (npos == 0) {
                    p = 0.1;
                } else if (npos == parents.size()) {
                    p = 0.9;
                } else {
                    p = npos / parents.size();
                }
                set_node_probability(*bn, nodeIndex, 1, parent_state, p);
                set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);
            }
        }
    } else if (probabilityMode == "or") {
        if (parents.size() == 0) {
            double p = 0.6;
            assignment parent_state;
            set_node_probability(*bn, nodeIndex, 1, parent_state, p);
            set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);
        } else {
            for (int i = 0; i < tableAssignments.size(); i++) {
                assignment parent_state;
                int npos = 0;
                for (int j = 0; j < tableAssignments[i].size(); j++) {
                    parent_state.add(nodeLabels[parents[j]], tableAssignments[i][j]);
                    if (tableAssignments[i][j] == 1) {
                        npos++;
                    }
                }
                double p;
                if (npos == 0) {
                    p = 0.4;
                } else if (npos == parents.size()) {
                    p = 0.6;
                } else {
                    p = 0.5 + (0.2 * npos / parents.size());
                }
                set_node_probability(*bn, nodeIndex, 1, parent_state, p);
                set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);
            }
        }
    } else if (probabilityMode == "dominantParent") {
        for (int i = 0; i < tableAssignments.size(); i++) {
            assignment parent_state;
            bool dominantParentTrue = false;

            int npos = 0;
            for (int j = 0; j < tableAssignments[i].size(); j++) {
                parent_state.add(nodeLabels[parents[j]], tableAssignments[i][j]);
                if (tableAssignments[i][j] == 1) {
                    if (dominantParentIndex == j) {
                        dominantParentTrue = true;
                    }
                    npos++;
                }
            }
            double p;
            if (dominantParentTrue) {
                p = 0.8;
            } else {
                p = 0.3;
            }

            set_node_probability(*bn, nodeIndex, 1, parent_state, p);
            set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);
        }
    }
    //    cout << "CPT for node " << node << " " << nodeIndex << " = " << node_cpt_filled_out(*bn, nodeIndex) << "\n";
    //    for (int i = 0; i < parents.size(); i++) {
    //        cout << parents[i] << " ";
    //    }
    //    if (parents.size() > 0) cout << "\n";
    conditional_probability_table * table = (&bn->node(nodeIndex).data.table());

    if (parents.size() > 0) {
        for (int i = 0; i < tableAssignments.size(); i++) {
            assignment parent_state;
            for (int j = 0; j < tableAssignments[i].size(); j++) {
                parent_state.add(nodeLabels[parents[j]], tableAssignments[i][j]);
            }
            //            cout << table->probability(0, parent_state) << "\n";
            //            cout << table->probability(1, parent_state) << "\n";
        }
    } else {
        assignment parent_state;
        //        cout << table->probability(0, parent_state) << "\n";
        //        cout << table->probability(1, parent_state) << "\n";
    }
}

bool BayesianNetwork::addNode(string node, std::vector<string> parents, std::vector<probAssignment> probTable) {
    //create new node
    lastIndex = lastIndex + 1;
    int nodeIndex = lastIndex;

    nodeLabels[node] = nodeIndex;
    set_node_num_values(*bn, nodeIndex, 2);
    numValues.push_back(2);

    std::map<string, int> actualValues;
    actualValues["f"] = 0;
    actualValues["t"] = 1;
    nodeValues[node] = actualValues;

    //add parent edges
    for (string parent : parents) {
        bn->add_edge(nodeLabels[parent], nodeIndex);
    }

    //        cout << "CPT for node " << node << " " << nodeIndex << "\n";
    //        for (int i = 0; i < parents.size(); i++) {
    //            cout << parents[i] << " ";
    //        }
    //        cout << "\n";

    for (probAssignment row : probTable) {
        assignment parentState;
        for (auto parentValue : row.parentValues) {
            int parentIndex = nodeLabels[parentValue.first];
            int value = nodeValues[parentValue.first][parentValue.second];
            parentState.add(parentIndex, value);

            //            cout<<parentValue.first<<" "<<parentValue.second<<" ";
        }
        //        cout<<"\n";
        set_node_probability(*bn, nodeIndex, 1, parentState, row.prob);
        set_node_probability(*bn, nodeIndex, 0, parentState, 1 - row.prob);
        //                cout << row.prob << "\n";
    }
    //        for (auto a:nodeValues[node]) {
    //            cout<<a.first<<" "<<a.second<<"\n";
    //        }
}

bool BayesianNetwork::addExclusiveOrNode(string node, std::vector<string> parents) {
    //create new node
    lastIndex = lastIndex + 1;
    int nodeIndex = lastIndex;

    nodeLabels[node] = nodeIndex;
    set_node_num_values(*bn, nodeIndex, 2);
    numValues.push_back(2);

    std::map<string, int> actualValues;
    actualValues["f"] = 0;
    actualValues["t"] = 1;
    nodeValues[node] = actualValues;

    //add parent edges
    for (string parent : parents) {
        bn->add_edge(nodeLabels[parent], nodeIndex);
    }

    //    cout << "CPT for node " << node << " " << nodeIndex << "\n";
    //    for (int i = 0; i < parents.size(); i++) {
    //        cout << parents[i] << " ";
    //    }
    //    cout << "\n";


    std::vector<std::vector<int> > tableVariables;
    for (string par : parents) {
        std::map<string, int> nodeValues = getValues(par);
        std::vector<int> values;
        for (int i = 0; i < nodeValues.size(); i++) {
            values.push_back(i);
        }
        tableVariables.push_back(values);
    }
    //get the matrix corresponding to the intentions combination table
    std::vector<std::vector<int> > tableAssignments;
    NestedLoop<int> loop(tableVariables);
    tableAssignments = loop.buildMatrix();
    for (auto row : tableAssignments) {
        double p;

        assignment parent_state;

        int n_true = 0;
        for (int j = 0; j < row.size(); j++) {
            parent_state.add(nodeLabels[parents[j]], row[j]);
            if (row[j] == 1) {
                n_true++;
            }
        }

        //        p = n_true == 1 ? 1 : 0;
        p = n_true == 1 ? 0.9 : 0.1;

        set_node_probability(*bn, nodeIndex, 1, parent_state, p);
        set_node_probability(*bn, nodeIndex, 0, parent_state, 1 - p);


    }

}
