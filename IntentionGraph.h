/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   IntentionGraph.h
 * Author: mfiore
 *
 * Created on August 12, 2016, 10:58 AM
 */

#ifndef INTENTIONGRAPH_H
#define INTENTIONGRAPH_H

#include "BayesianNetwork.h"
#include "Mdp.h"


struct IntentionNode {
    string name;
    std::vector<string> linked_contexts;
    double influence;
};


class IntentionGraph {
public:
    IntentionGraph();
    IntentionGraph(const IntentionGraph& orig);
    virtual ~IntentionGraph();
    
    void setGraph(std::vector<string> contexts, std::vector<IntentionNode> intention, std::vector<string> actions, 
    std::vector<Mdp*> mdps, VariableSet state);
    
    void computeProbability(VariableSet evidence);
    
private:
    BayesianNetwork *bn;
    
    void createActionNodes(std::vector<string> actions, std::vector<string> intention_list, std::vector<Mdp*> mdps, VariableSet state);

};

#endif /* INTENTIONGRAPH_H */

