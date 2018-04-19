/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Data.cpp
 * Author: XIM33
 * 
 * Created on April 9, 2018, 3:34 PM
 */
#include <stdlib.h>
//#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include <string>
#include "Data.h"

using namespace std;


//Data::Data() {
//}
Data::Data(string pFileName, string iFileName, string edgeFileName){
    readinMatrix(pFileName, 'P');
    readinMatrix(iFileName, 'i');
    readinEdges(edgeFileName);
    buildNetwork();
    for (int i = 0; i < combinedMatrix.size(); i++){
        cout << combinedMatrix[i]<<",";
    }
    cout << "\n";
    
}
Data::Data(const Data& orig) {
}

Data::~Data() {
    for(int i = 0; i < nodeList.size(); i++){
        delete nodeList[i];
    }
}

void Data::readinMatrix(string fileName,char type)
{   
    stringstream ss;
    string line;
    ifstream inFileStream;   
    vector<unsigned int*> matrixAsVec;
    int nCol = 0, nRow = 0;

    try
    {
        inFileStream.open(fileName.c_str());
        if ( (inFileStream.rdstate() & ifstream::failbit ) != 0 )
        {
            cerr << "Error opening file when loading " + fileName + ", quit.\n";
            inFileStream.close();
            exit(EXIT_FAILURE);
        }
    }
    catch (...) 
    { //std::ifstream::failure e
        cerr << "Fail to open file " << fileName;
        
    }

    //cout << "Opened GE file ok.\n";
            
    // read in first line to figure out the number of columns and column Names;
    getline(inFileStream, line);
//    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
//    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    if (!line.empty() && line[line.size() - 1] == '\r')
             line.erase(line.size() - 1);
    string tmp;
    stringstream firstSS(line);

    bool firstColFlag = true;
    while (getline(firstSS, tmp, ',' ))
    {
        if(firstColFlag)
        {
            firstColFlag = false;
            continue;
        }
        if (type == 'P'){//save protein name when read in phosphorylation matrix
            proteinNames.push_back(tmp);
        }
        nCol++;
    }

    while (getline(inFileStream, line))
    {
        firstColFlag = true; 

        stringstream anotherss(line);
        string tmp;
        int curCol = 0;
        while (getline(anotherss, tmp, ','))
        {
            if(firstColFlag)
            {
                firstColFlag = false;
                if (type == 'P'){//save case name when read in phosphorylation matrix
                    caseNames.push_back(tmp);
                }
                matrixAsVec.push_back(new unsigned int[nCol]());
                continue;
            }

            matrixAsVec[nRow][curCol] = atoi(tmp.c_str());
            curCol++;
        }        
        nRow++;		
    }

    inFileStream.close();          
    
    if (type == 'P'){ //if it is phosphorylation,then we initialize activation, and combine activation and phosphorylation into one table  
         
        //build allNodesTable as a  consecutive array so that computation can be done efficiently
        //allNodesTable consists of activation table as first nCol, and phosphorylation table as second nCol
//        combinedMatrix = new unsigned int[2*nCol*nRow]();
        //First nCol is initial activation table which was generated by random 0/1 number
        for (int c = 0; c < nCol; c++){
            for (int r = 0; r < nRow; r++){
                combinedMatrix.push_back(rand()%2);
//                combinedMatrix[indx++] = rand()%2;
            }
        }
        //Second nCol is a copy from phosphorylation table
        for (int c = 0; c < nCol; c++){
            for (int r = 0; r < nRow; r++){
                combinedMatrix.push_back(matrixAsVec[r][c]);
//                combinedMatrix[indx++] = matrixAsVec[r][c]; 
            }
        }

        //Initialize ActivMatrix for infer
//        activatMatrix = new float*[nCol*nRow]();

        nProtein = nCol;
        nCase = nRow;
    }

    else{
//        intervMatrix = new int[nCol*nRow]();
//        int indx = 0;
        for (int c = 0; c < nCol; c++){
            for (int r = 0; r < nRow; r++){
//                intervMatrix[indx++] =  matrixAsVec[r][c];
                intervMatrix.push_back(matrixAsVec[r][c]);
            }
        }        
    }

    
    
    // free temporary matrix
    for (int i = 0; i < matrixAsVec.size(); i++) 
        delete [] matrixAsVec[i];
}

void Data::readinEdges(string edgeFileName){
    ifstream inFileStream;
    string line;
    vector<string> fields;
    
   
    try {
        inFileStream.open(edgeFileName.c_str()); 
 
        while(getline(inFileStream, line))
        {   
//            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end()); 
//            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end()); 
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            edges.push_back(line);
        }
        inFileStream.close();
    }
    catch (ifstream::failure e) {
        cout << "Fail to open file " << edgeFileName;
        
    } 
}


void Data::buildNetwork(){
    //build nodeList
    //add activation node to nodeList
    for (int i = 0; i < nProtein; i++ ){
        string name = proteinNames[i];
        Node* node = new Node(name,'A',i);
        nodeList.push_back(node);
        //also build the nodeListMap
        nodeListMap.insert(std::pair<string, int>(name, i));
    }
    //add phosphorylation node to nodeList
    for (int i = 0; i < nProtein; i++ ){
        nodeList.push_back(new Node(proteinNames[i],'P',i + nProtein));
    }
    //build the parent and children of each node
    //implicitly phosph node is the parent of its activation node
    for(int i = 0; i < nProtein; i++){
        nodeList[i]->addToParents(i + nProtein);  //corresponding phosph node is the parent of activation node
    }
    for(int i = nProtein; i < 2*nProtein; i++){
        nodeList[i]->addToChildren(i - nProtein); //corresponding activation node is the child of phosph node
    }
    //further build the parent and children of the node based on the EdgeList
    for(int i = 0; i < edges.size(); i++){
        string edge = edges[i];
        int p = edge.find(',');
        string headname = edge.substr(0,p); //activation
        string tailname = edge.substr(p+1); //phosphorylation
        int headNodeIndex = nodeListMap[headname];
        //suppose head is activNode and tail is phosphNode
        int tailNodeIndex = nodeListMap[tailname] + nProtein;
        
        nodeList[headNodeIndex]->addToChildren(tailNodeIndex);
        nodeList[tailNodeIndex]->addToParents(headNodeIndex);
    }

 
}
double Data::getCPTvalueOfANode(Node* Anode, int caseNumber){
    vector<double> ACPTvalue(2, 0.0);
    getCPTvalueOfANode(Anode, caseNumber, ACPTvalue);
    int nodeIndex = Anode->getIndex();
    int Avalue = combinedMatrix[nCase * nodeIndex + caseNumber];
    return ACPTvalue[Avalue];
}

void Data::getCPTvalueOfANode(Node* Anode, int caseNumber, vector<double>& inferValueOfNode ){
//    int nodeIndex = Anode->getIndex();
//    double inferValueOfNode[2] = {0.0};

    //get values of nodeA parents
    vector<int>& parentIndex = Anode->getParents();
    int numOfParents = parentIndex.size();
    vector<int> parentValues;
    for (int p = 0; p < numOfParents; p++){
        parentValues.push_back(combinedMatrix[nCase * parentIndex[p] + caseNumber]);
    }

    //lookup CPT
    vector<double>& CPT1 = Anode->getCPT();
    int lookupIndex = 0;
    for (int p = numOfParents - 1; p >=0 ; p--){
        lookupIndex += pow(2,p) * parentValues[p];
    }

    inferValueOfNode[1] = CPT1[lookupIndex];
    inferValueOfNode[0] = 1 - CPT1[lookupIndex];

}

void Data::getCPTvalueOfOneChildOfANode(Node* node, Node* inferNode,int caseNumber, vector<double>& inferValueOfNode){
    int nodeIndex = node->getIndex();
    int inferNodeIndex = inferNode->getIndex();
    
//    double inferValueOfNode[2] = {0.0};//[0] save the CPT value for inferNodeValue==0; [1] save the CPT value for inferNodeValue==1
  
    int nodeValue = combinedMatrix [ nCase * nodeIndex + caseNumber];

    //get values of nodeA parents
    vector<int> parentIndex = node->getParents();
    int numOfParents = parentIndex.size();
    vector<int> parentValues0, parentValues1;
    for (int p = 0; p < numOfParents; p++){
        if (parentIndex[p] == inferNodeIndex){
            parentValues1.push_back(1);
            parentValues0.push_back(0);
        }else{
            parentValues1.push_back(combinedMatrix[nCase * parentIndex[p] + caseNumber]);
            parentValues0.push_back(combinedMatrix[nCase * parentIndex[p] + caseNumber]);
        }
    }

    //lookup CPT
    vector<double>& CPT1 = node->getCPT();
    int lookupIndex0 = 0, lookupIndex1 = 0;
    for (int p = numOfParents - 1; p >=0 ; p--){
        lookupIndex1 += pow(2,p) * parentValues1[p];
        lookupIndex0 += pow(2,p) * parentValues0[p];
    }
    
    if (nodeValue == 1){
        inferValueOfNode[1] = CPT1[lookupIndex1];
        inferValueOfNode[0] = CPT1[lookupIndex1];
    }else{
        inferValueOfNode[1] = CPT1[lookupIndex0];
        inferValueOfNode[0] = CPT1[lookupIndex0];
    }
    
}

void Data::calCPTofEachNode(){
    //calculate CPT for each node
    for(int i = 0; i < 2*nProtein; i++){
        nodeList[i]->calculateCPT(combinedMatrix,nProtein*2, nCase);
    }
    for (int i = 0; i < nodeList.size(); i++){
        vector<double>& CPT1 =nodeList[i]->getCPT();
        for (int j = 0; j < CPT1.size(); j++){
            cout << CPT1[j] << ",";
        }
        cout << "\n";
    }
}
void Data::inferActivation(){
    
    activatMatrix.clear();
    //loop through each protein and each case to infer activatin(lookup in CPT of each node)
    double inferValue = 0.0;
    for (int p = 0; p < nProtein; p++){
        Node* Anode = nodeList[p];
        vector<int>& Achildren = Anode->getChildren();
        for (int c = 0; c < nCase; c++){
            //get intervention value of the node
            int Vinterv = intervMatrix[p*nCase + c];
            if (Vinterv == 1 or Vinterv == 0) //if has intervention, no infer
                inferValue = Vinterv;
            else{ //infer
                
                //get CPT values of A node
                
                vector<double> ACPTvalue(2, 0.0);
//                getCPTvalueOfANode(Anode,c, ACPTvalue);
                getCPTvalueOfANode(Anode,c, ACPTvalue);
                //get CPT values of A children's nodes
                vector<vector<double> >  AchildrenCPTvalue;
                
                for (int i = 0; i < Achildren.size() ; i++){
                    vector<double> childCPTvalue(2,0.0);
                    getCPTvalueOfOneChildOfANode(nodeList[i], Anode, c, childCPTvalue );
                    AchildrenCPTvalue.push_back(childCPTvalue);
                }
                
                /*
                 * calculate p(A|.)=1/(1+p(A=0)/p(A=1) 
                 *                = 1/(1 + exp( lg(p(A=0)) - lg(p(A=1)) ) )
                 * lg(p(A=0)) = lg(P(A=0|pa)) + Sigma[ lg(P(C|Cpa)]for A=0
                 *lg(p(A=1)) = lg(P(A=1|pa)) + Sigma[ lg(P(C|Cpa)]for A=1
                */
                
                double lgA0pa = log(ACPTvalue[0]);
                double lgA1pa = log(ACPTvalue[1]);
                
                double lgChildrenA0 = 0.0, lgChildrenA1 = 0.0;
                int numOfChildren = AchildrenCPTvalue.size();
                for (int i = 0; i< numOfChildren; i++){
                    lgChildrenA0 += log(AchildrenCPTvalue[p][0]);
                    lgChildrenA1 += log(AchildrenCPTvalue[p][1]);
                }
                
                double lgA0= lgA0pa + lgChildrenA0;
                double lgA1= lgA1pa + lgChildrenA1;
                
                inferValue = 1/(1+exp(lgA0 - lgA1));
            }
            //generate a random number r
            double r = (double)rand() / (double)RAND_MAX;
            if (inferValue < r){
                //set activationNode in combinedMatrix 
                combinedMatrix[p*nCase + c] = 0;
            }
            else  {
                combinedMatrix[p*nCase + c] = 1;
            }
        }
//            activatMatrix.push_back(inferValue);
        
    }
}

double Data::calJointProbOfAllNodes(){
    
    for(int c = 0; c < nCase; c++){
        double logJointProbofAcase = 0.0;
        for(int p = 0;  p < nodeList.size(); p++){
            double probOfNode = getCPTvalueOfANode(nodeList[p], c );
            logJointProbofAcase += log(probOfNode);
        }
    }
    return 0.0;
}

