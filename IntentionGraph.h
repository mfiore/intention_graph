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
 * 
 * creates an Intention Graph, using the Bayesian NEtwork class
 */

#ifndef INTENTIONGRAPH_H
#define INTENTIONGRAPH_H

#include "BayesianNetwork.h"
#include "Mdp.h"
#include <map>

struct IntentionNode { //structure to represent intention nodes
    string name;  //name of the intention
    std::vector<string> linked_contexts; //contexts that influence it
    std::vector<double> influence; //influence of these contexts
}; 


class IntentionGraph {
public:
    //If approximation loops=0 it will use exact methods to compute probabilities, if not it will use a gibbs sampler
    //with a number of loops equal to approximation_loops (more loops mean more accuracy but also slower response) 
    IntentionGraph(int approximation_loops);
    IntentionGraph(const IntentionGraph& orig);
    virtual ~IntentionGraph();
    
    //creates the graph, receiving a list of context, a lit of intention nodes, a list of actions,  a set of MDPs, one for each intention,
    //and system state
    void setGraph(std::vector<string> contexts, std::vector<IntentionNode> intention, std::vector<string> actions, 
    std::vector<Mdp*> mdps, VariableSet state);
    
    //computes the probability in the graph, returning a map linking a node to its prob
    std::map<string,double> computeProbability(VariableSet evidence);
    
    //getter for nodes
    std::vector<std::string> getObservationNodes();
    std::vector<std::string> getContextNodes();
    std::vector<std::string> getActionNodes();
    
private:
    BayesianNetwork *bn;
   
    std::vector<string> intentions_;
    std::vector<string> actions_;
    
    //creats the action nodes, using hte MDPs
    void createActionNodes(std::vector<string> actions, std::vector<string> intention_list, std::vector<Mdp*> mdps, VariableSet state);

    std::vector<std::string> observation_nodes_;
    std::vector<std::string> context_nodes_;

    std::map<std::string, int> cost_intention_;
    std::map<std::string,bool> has_decreased_cost_;
    
    int n_approx;
};

#endif /* INTENTIONGRAPH_H */

