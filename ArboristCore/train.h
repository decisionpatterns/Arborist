// This file is part of ArboristCore.

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
   @file train.h

   @brief Class definitions for the training entry point.

   @author Mark Seligman
 */

#ifndef ARBORIST_TRAIN_H
#define ARBORIST_TRAIN_H

#include <vector>
//using namespace std;

/**
   @brief Interface class for front end.  Holds simulation-specific parameters
   of the data and constructs forest, leaf and diagnostic structures.
*/
class Train {
  static const double slopFactor = 1.2; // Estimates tree growth.
 protected:
  static int trainBlock; // Front-end defined buffer size.
  static unsigned int nTree;
  static unsigned int nRow;
  static int nPred;

  class Forest *forest;
  std::vector<unsigned int> &inBag;
  double *predInfo; // E.g., Gini gain:  nPred.

  static void DeImmutables();
  Train(std::vector<unsigned int> &_inBag, std::vector<unsigned int> &_orig, std::vector<unsigned int> &_facOrig, double _predInfo[], std::vector<class ForestNode> &_forestNode, std::vector<unsigned int> &_facSplit);

  virtual ~Train();
  void ForestTrain(const class RowRank *rowRank);
  void BagSetTree(const class BV *treeBag, class BitMatrix *forestBag, int treeNum);

 public:
/**
   @brief Static initializer.

   @return void.
 */
  static void Init(double *_feNum, int _facCard[], int _cardMax, int _nPredNum, int _nPredFac, int _nRow, int _nTree, int _nSamp, double _feSampleWeight[], bool withRepl, int _trainBlock, int _minNode, double _minRatio, int _totLevels, int _ctgWidth, int _predFixed, double _predProb[], int _regMono[] = 0);

  static void Regression(int _feRow[], int _feRank[], int _feInvNum[], double _y[], double _yRanked[], std::vector<unsigned int> &_inBag, std::vector<unsigned int> &_orig, std::vector<unsigned int> &_facOrig, double _predInfo[], std::vector<class ForestNode> &_forestNode, std::vector<unsigned int> &_facSplit, std::vector<unsigned int> &_rank, std::vector<unsigned int> &_sCount);

  static void Classification(int _feRow[], int _feRank[], int _feInvNum[], int _yCtg[], int _ctgWidth, double _yProxy[], std::vector<unsigned int> &_inBag, std::vector<unsigned int> &_orig, std::vector<unsigned int> &_facOrig, double _predInfo[], std::vector<class ForestNode> &_forestNode, std::vector<unsigned int> &_facSplit, std::vector<double> &_weight);

  void Reserve(class PreTree **ptBlock, int tCoun);
  int BlockPeek(class PreTree **ptBlock, int tCount, int &blockFac, int &blockBag, int &maxHeight);
  void BlockTree(class PreTree **ptBlock, int tStart, int tCount);
  virtual void FactoryLeaf(int heightEst, int bagEst) = 0;
  virtual void Block(const class RowRank *rowRank, class BitMatrix *forestBag, int tStart, int tCount) = 0;
};


class TrainReg : public Train {
  std::vector<unsigned int> &rank;
  std::vector<unsigned int> &sCount;
  class LeafReg *leafReg;
  class ResponseReg *responseReg;
  void FactoryLeaf(int heightEst, int bagEst);
  void Block(const class RowRank *rowRank, class BitMatrix *forestBag, int tStart, int tCount);
  void BlockLeaf(class PreTree **ptBlock, class SampleReg **sampleBlock, class BitMatrix *forestBag, int tSTart, int tCount);
 public:
  TrainReg(double _y[], double _yRanked[], std::vector<unsigned int> &_inBag, std::vector<unsigned int> &_orig, std::vector<unsigned int> &_facOrig, double _predInfo[], std::vector<class ForestNode> &_forestNode, std::vector<unsigned int> &_facSplit, std::vector<unsigned int> &_rank, std::vector<unsigned int> &_sCount);
  ~TrainReg();
};


class TrainCtg : public Train {
  const unsigned int ctgWidth;
  std::vector<double> &weight;
  class LeafCtg *leafCtg;
  class ResponseCtg *responseCtg;
  void FactoryLeaf(int heightEst, int bagEst);
  void Block(const class RowRank *rowRank, class BitMatrix *forestBag, int tStart, int tCount);
  void BlockLeaf(class PreTree **ptBlock, class SampleCtg **sampleBlock, class BitMatrix *forestBag, int tStart, int tCount);
 public:
  TrainCtg(int _yCtg[], unsigned int _ctgWidth, double _yProxy[], std::vector<unsigned int> &_inBag, std::vector<unsigned int> &_orig, std::vector<unsigned int> &_facOrig, double _predInfo[], std::vector<class ForestNode> &_forestNode, std::vector<unsigned int> &_facSplit, std::vector<double> &_weight);
  ~TrainCtg();
};

#endif
