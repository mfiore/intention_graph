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
    
    intentions_=intentions;
    actions_=actions;
    
    int bn_size = contexts.size() + intentions.size() + actions.size() + actions.size()*2;
    if (bn != NULL) {
        delete bn;
    }
    bn = new BayesianNetwork(bn_size);

    for (string c : contexts) {
        bn->addNode(c, std::vector<string>(), false, "uniform", "");
    }

    std::vector<string> intention_list;
    for (int i = 0; i < intentions.size(); i++) {
        bn->addNode(intentions[i].name, intentions[i].linked_contexts, false, "or", "");
        intention_list.push_back(intentions[i].name);
    }

    createActionNodes(actions, intention_list, mdps, state);

    std::vector<string> distanceValues;
    distanceValues.push_back("out");
    distanceValues.push_back("far");
    distanceValues.push_back("medium");
    distanceValues.push_back("close");
    distanceValues.push_back("reach");

    for (string action : actions) {
        std::vector<string> parents = {action};
        bn->addMultiValueNode("distance_" + action, action, distanceValues);
        bn->addNode("toward_" + action, parents, false, "or", "");
    }

    bn->updateJoinTree();
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

    cout << "Action Values\n";
    std::vector<double> sumq(intention_list.size(), 0);
    for (string a : actions) {
        for (int i = 0; i < mdps.size(); i++) {
            double q = mdps[i]->getQValue(state, a);
            //            cout << intentions[i] << " " << a << " " << q << "\n";
            sumq[i] = sumq[i] + q;
        }
    }
    std::vector<string> intention_values = {"f", "t"};
    for (string a : actions) {
        std::vector<probAssignment> probTable;
        bool firstRow = true;
        for (auto row : tableAssignments) {
            probAssignment probRow;
            double qrow = 0;
            double totSum = 0;
            for (int i = 0; i < intention_list.size(); i++) {
                probRow.parentValues[intention_list[i]] = intention_values[row[i]];
                if (row[i] == 1) {
                    qrow = qrow + mdps[i]->getQValue(state, a);
                    totSum = totSum + sumq[i];
                }
            }
            if (totSum > 100) {
                probRow.prob = 0;
            } else {
                probRow.prob = 1-(qrow / totSum);
            }
            probTable.push_back(probRow);
        }
        bn->addNode(a, intention_list, probTable);
    }
}

std::map<string,double> IntentionGraph::computeProbability(VariableSet evidence) {
   std::map<NodeAssignment,double> probability=bn->computeProbability(evidence.set);

   std::map<string,double> result;
   for (std::map<NodeAssignment,double>::iterator i=probability.begin();i!=probability.end();i++) {
       if (i->first.value==1) {
           result[i->first.name]=i->second;
       }
   }
   return result;
}