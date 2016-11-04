/* 
 * File:   BayesianNetwork.h
 * Author: mfiore
 *
 * Created on December 15, 2014, 9:47 AM
 */

#ifndef BAYESIANNETWORK_H
#define BAYESIANNETWORK_H

#include <dlib/bayes_utils.h>
#include <dlib/graph_utils.h>
#include <dlib/graph.h>
#include <dlib/directed_graph.h>
#include <iostream>
#include <boost/foreach.hpp>
#include <vector>
#include "NestedLoop.h"
#include <map>
using namespace dlib;
using namespace std;
using namespace bayes_node_utils;

typedef graph<dlib::set<unsigned long>::compare_1b_c, dlib::set<unsigned long>::compare_1b_c>::kernel_1a_c join_tree_type;

struct probAssignment {
    std::map<string, string> parentValues;
    string value;
    double prob;
};

struct NodeAssignment {
    string name;
    int value;

    bool operator<(const NodeAssignment &o) const {
        return name < o.name || name == o.name && value < o.value;
    }
};

class BayesianNetwork {
public:
    //creates a BN. If approximation loops=0 it will use exact methods to compute probabilities, if not it will use a gibbs sampler
    //with a number of loops equal to approximation_loops (more loops mean more accuracy but also slower response)
    BayesianNetwork(int size, int approximation_loops);
    BayesianNetwork(const BayesianNetwork& orig);

    virtual ~BayesianNetwork();


    void updateJoinTree();

    string getLabel(int i);
    int getIndex(string label);
    std::map<string, int> getValues(int i);
    std::map<string, int> getValues(string label);
   std::map<NodeAssignment, double> computeProbability(std::map<string, string> nodeValues);

   //all these functions are wrappers to dlib to create particular kind of nodes. 
   //Probability mode can assume the values:
   //uniform (each value of the node has the same prob)
   //equal weight (parents have the same strenght in influencing the node)
   //or (the more parents are 1 the higher is the prob)
   //dominantParent (one parent, included in the dominantParent input, has a bigger influence than the other) 
    bool addNode(string node, std::vector<string> parents,  string probabilityMode, string dominantParent);

    //this one receives the full prob table
    bool addNode(string node, std::vector<string> parents, std::vector<probAssignment> probTable);

    //this one receives just an influence value. Meaning when the parent is true the conditional prob will be influence, else 0.5
    bool addNode(string node, std::vector<string> parents, std::vector<double> influence);

    //this adds a node with more than one value
    bool addMultiValueNode(string node, string parent, std::vector<string> values);

    //this add a node to work as an XOR in the diagram
    bool addExclusiveOrNode(string node, std::vector<string> parents);

private:
    directed_graph<bayes_node>::kernel_1a_c *bn;
    join_tree_type join_tree;
    std::map<string, int> nodeLabels;
    std::map<string, std::map<string, int> > nodeValues;
    std::vector<bool> isTemporal;

    std::vector<int> numValues;

    
    int lastIndex;
    
    int approximation_loops;
};

#endif /* BAYESIANNETWORK_H */

