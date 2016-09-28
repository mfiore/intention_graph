/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   IntentionGraph.cpp
 * Author: mfiore
 * 
 * Created on August 12, 2016, 10:58 AM
 */

#include "IntentionGraph.h"

IntentionGraph::IntentionGraph() {
    bn = NULL;
}

IntentionGraph::IntentionGraph(const IntentionGraph& orig) {
}

IntentionGraph::~IntentionGraph() {
}

void IntentionGraph::setGraph(std::vector<string> contexts, std::vector<IntentionNode> intentions, std::vector<string> actions,
        std::vector<Mdp*> mdps, VariableSet state) {

    //    intentions_=intentions;
    //    actions_=actions;
    context_nodes_ = contexts;

    int bn_size = contexts.size() + intentions.size() + actions.size() + actions.size()*2+2;
    if (bn != NULL) {
        delete bn;
    }
    bn = new BayesianNetwork(bn_size);

    for (string c : contexts) {
        bn->addNode(c, std::vector<string>(), false, "uniform", "");
    }

    std::vector<string> intention_list;
    for (int i = 0; i < intentions.size(); i++) {
        bn->addNode(intentions[i].name, intentions[i].linked_contexts, intentions[i].influence);
        intention_list.push_back(intentions[i].name);
    }

    //intention or node
    bn->addExclusiveOrNode("IntentionsOr", intention_list);
    createActionNodes(actions, intention_list, mdps, state);
    bn->addExclusiveOrNode("ActionsOr", actions);
    std::vector<string> distanceValues;
    distanceValues.push_back("out");
    distanceValues.push_back("far");
    distanceValues.push_back("medium");
    distanceValues.push_back("close");
    distanceValues.push_back("reach");

    for (string action : actions) {
        std::vector<string> parents = {action};
        bn->addMultiValueNode("distance_" + action, action, distanceValues);
        bn->addNode("deltaDistance_" + action, parents, false, "dominantParent", action);
        observation_nodes_.push_back("distance_" + action);
        observation_nodes_.push_back("deltaDistance_" + action);
    }
    bn->updateJoinTree();
}

double getUtility(double this_q, std::vector<double> all_q, bool positive) {
    int n_parts = 1;
    int counter = 1;
    int this_counter = 0;
    if (this_q >= 1000) return 0;
    if (positive) {
        for (int i = all_q.size() - 2; i >= 0; i--) {
            if (all_q[i] != all_q[i + 1]) {
                counter = counter + 2;
            }
            if (all_q[i] == this_q) {
                this_counter = counter;
            }
            n_parts = n_parts + counter;
        }
    } else {
        return 0;
        //        for (int i = 1; i < all_q.size(); i++) {
        //            if (all_q[i] != all_q[i - 1]) {
        //                counter++;
        //            }
        //            if (all_q[i] == this_q) {
        //                this_counter = counter;
        //            }
        //            n_parts = n_parts + counter;
        //    }s
    }
    if (this_counter == 0) this_counter = 1;
    double acc = (double) 1 / n_parts;
    return acc*this_counter;
}

void IntentionGraph::createActionNodes(std::vector<string> actions, std::vector<string> intention_list, std::vector<Mdp*> mdps, VariableSet state) {
    std::vector<std::vector<int> > tableVariables;
    for (string intention : intention_list) {
        std::map<string, int> nodeValues = bn->getValues(intention);
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


    std::vector<std::vector<std::string> > mdp_actions;
    for (int i = 0; i < intention_list.size(); i++) {
        std::vector<string> par_actions;
        for (string a : actions) {
            string pa = mdps[i]->getParametrizedAction(a);
            if (std::find(mdps[i]->actions_.begin(), mdps[i]->actions_.end(), pa) != mdps[i]->actions_.end()) {
                par_actions.push_back(pa);
            }
        }
        mdp_actions.push_back(par_actions);
    }

    std::vector<double> sumq(intention_list.size(), 0);
    for (int i = 0; i < mdps.size(); i++) {
        for (string a : mdp_actions[i]) {
            double q = mdps[i]->getQValue(state, a);
            //            cout << intentions[i] << " " << a << " " << q << "\n";
            sumq[i] = sumq[i] + q;
        }
    }
    std::map<string, std::vector<double> > all_q_intentions;
    for (int i = 0; i < intention_list.size(); i++) {
        std::vector<double> all_q;
        for (string a : mdp_actions[i]) {
            all_q.push_back(mdps[i]->getQValue(state, a));
        }
        std::sort(all_q.begin(), all_q.end());
        all_q_intentions[intention_list[i]] = all_q;
    }

    std::vector<string> intention_values = {"f", "t"};
    for (string a : actions) {
        std::vector<probAssignment> probTable;
        bool firstRow = true;
        for (auto row : tableAssignments) {
            probAssignment probRow;
            double qrow = 0;
            double totSum = 0;
            double utility = 0;
            std::vector<double> single_utilities(intention_list.size(), 0);
            for (int i = 0; i < intention_list.size(); i++) {
                string par_a = mdps[i]->getParametrizedAction(a);
                probRow.parentValues[intention_list[i]] = intention_values[row[i]];
                std::vector<string> mdp_actions = mdps[i]->actions_;
                if (std::find(mdp_actions.begin(), mdp_actions.end(), par_a) == mdp_actions.end()) {
                    single_utilities[i] == 0;
                } else {
                    if (row[i] == 1) {
                        //                    qrow = qrow + mdps[i]->getQValue(state, a);
                        //                    totSum = totSum + sumq[i];
                        //                    single_utilities[i] = q == 1000 ? 0 : 1 - q / sumq[i];

                        double this_q = mdps[i]->getQValue(state, par_a);
                        single_utilities[i] = getUtility(this_q, all_q_intentions[intention_list[i]], true);

                    } else {
                        //                    qrow=qrow+1000;
                        //                    totSum=totSum+1000;
                        double this_q = mdps[i]->getQValue(state, par_a);
                        single_utilities[i] = getUtility(this_q, all_q_intentions[intention_list[i]], false);

                    }
                }
            }
            for (int i = 0; i < intention_list.size(); i++) {
                if (single_utilities[i] == 0 && row[i] == 1) {
                    utility = 0;
                    break;
                }
                utility = utility + single_utilities[i];
            }
            if (utility != 0) {
                double denom = 0;
                for (int i = 0; i < intention_list.size(); i++) {
                    if (row[i] == 1) {
                        for (string a : mdp_actions[i]) {
                            double this_q = mdps[i]->getQValue(state, a);
                            if (this_q < 1000) {
                                denom = denom + getUtility(this_q, all_q_intentions[intention_list[i]], true);
                            }
                        }
                    }
                }
                if (denom == 0) {
                    probRow.prob = 0;
                } else {
                    probRow.prob = utility / denom;
                }
            } else {
                probRow.prob = 0;
            }
            //            if (totSum == 0) {
            //                probRow.prob = 1;
            //            } else {
            //                probRow.prob = 1-(qrow / totSum);
            //            }
            probTable.push_back(probRow);
        }
        //        bn->addNode(a,intention_list,false,"uniform","");
        bn->addNode(a, intention_list, probTable);

    }
}

std::map<string, double> IntentionGraph::computeProbability(VariableSet evidence) {
    std::map<NodeAssignment, double> probability = bn->computeProbability(evidence.set);

    std::map<string, double> result;
    for (std::map<NodeAssignment, double>::iterator i = probability.begin(); i != probability.end(); i++) {
        if (i->first.value == 1) {
            result[i->first.name] = i->second;
        }
    }
    return result;
}

std::vector<std::string> IntentionGraph::getObservationNodes() {
    return observation_nodes_;
}

std::vector<std::string> IntentionGraph::getContextNodes() {
    return context_nodes_;
}

std::vector<std::string> IntentionGraph::getActionNodes() {
    return actions_;
}
