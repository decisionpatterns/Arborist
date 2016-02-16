// This file is part of ArboristCore.

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
   @file predict.cc

   @brief Methods for validation and prediction.

   @author Mark Seligman
 */

#include "predblock.h"
#include "forest.h"
#include "predict.h"
#include "quant.h"
#include "bv.h"

//#include <iostream>
using namespace std;


/**
   @brief Static entry for regression case.
 */
void Predict::Regression(double *_blockNumT, int *_blockFacT, unsigned int _nPredNum, unsigned int _nPredFac, std::vector<ForestNode> &_forestNode, std::vector<unsigned int> &_origin, std::vector<unsigned int> &_facOff, std::vector<unsigned int> &_facSplit, std::vector<double> &yPred, const std::vector<unsigned int> &_bag) {
  int nTree = _origin.size();
  unsigned int _nRow = yPred.size();
  PBPredict::Immutables(_blockNumT, _blockFacT, _nPredNum, _nPredFac, _nRow);
  PredictReg *predictReg = new PredictReg(nTree, _nRow);
  Forest *forest =  new Forest(_forestNode, _origin, _facOff, _facSplit);
  BitMatrix *bag = new BitMatrix(_nRow, nTree, _bag);
  predictReg->PredictAcross(forest, yPred, bag);

  delete bag;
  delete predictReg;
  delete forest;
  PBPredict::DeImmutables();
}


/**
   @brief Static entry for regression case.
 */
void Predict::Quantiles(double *_blockNumT, int *_blockFacT, unsigned int _nPredNum, unsigned int _nPredFac, std::vector<ForestNode> &_forestNode, std::vector<unsigned int> &_origin, std::vector<unsigned int> &_facOff, std::vector<unsigned int> &_facSplit, const std::vector<unsigned int> &rank, const std::vector<unsigned int> &sCount, const std::vector<double> &yRanked, std::vector<double> &yPred, const std::vector<double> &quantVec, unsigned int qBin, std::vector<double> &qPred, const std::vector<unsigned int> &_bag) {
  int nTree = _origin.size();
  unsigned int _nRow = yPred.size();
  PBPredict::Immutables(_blockNumT, _blockFacT, _nPredNum, _nPredFac, _nRow);
  PredictReg *predictReg = new PredictReg(nTree, _nRow);
  Forest *forest =  new Forest(_forestNode, _origin, _facOff, _facSplit);
  BitMatrix *bag = new BitMatrix(_nRow, nTree, _bag);
  Quant *quant = new Quant(forest, yRanked, rank, sCount, quantVec, qBin);
  predictReg->PredictAcross(forest, yPred, quant, &qPred[0], bag);

  delete bag;
  delete predictReg;
  delete quant;
  delete forest;
  
  PBPredict::DeImmutables();
}


/**
   @brief Entry for separate classification prediction.
 */
void Predict::Classification(double *_blockNumT, int *_blockFacT, unsigned int _nPredNum, unsigned int _nPredFac, std::vector<ForestNode> &_forestNode, std::vector<unsigned int> &_origin, std::vector<unsigned int> &_facOff, std::vector<unsigned int> &_facSplit, unsigned int _ctgWidth, double *_leafWeight, std::vector<int> &yPred, int *_census, int *_yTest, int *_conf, double *_error, double *_prob, const std::vector<unsigned int> &_bag) {
  int nTree = _origin.size();
  unsigned int _nRow = yPred.size();
  PBPredict::Immutables(_blockNumT, _blockFacT, _nPredNum, _nPredFac, _nRow);
  PredictCtg *predictCtg = new PredictCtg(nTree, _nRow, _ctgWidth, _leafWeight);
  Forest *forest = new Forest(_forestNode, _origin, _facOff, _facSplit);
  BitMatrix *bag = new BitMatrix(_nRow, nTree, _bag);
  predictCtg->PredictAcross(forest, bag, _census, yPred, _yTest, _conf, _error, _prob);

  delete predictCtg;
  delete forest;
  delete bag;
  PBPredict::DeImmutables();
}


PredictCtg::PredictCtg(int _nTree, unsigned int _nRow, unsigned int _ctgWidth, double *_leafWeight) : Predict(_nTree, _nRow), ctgWidth(_ctgWidth), leafWeight(_leafWeight) {
}


PredictReg::PredictReg(int _nTree, unsigned int _nRow)  : Predict(_nTree, _nRow) {
}


Predict::Predict(int _nTree, unsigned int _nRow) : nTree(_nTree), nRow(_nRow) {
  predictLeaves = new int[rowBlock * nTree];
}


Predict::~Predict() {
  delete [] predictLeaves;
}


void PredictCtg::PredictAcross(Forest *forest, BitMatrix *bag, int *census, std::vector<int> &yPred, int *yTest, int *conf, double *error, double *prob) {
  double *votes = new double[nRow * ctgWidth];
  for (unsigned int i = 0; i < nRow * ctgWidth; i++)
    votes[i] = 0;
  unsigned int rowStart;
  for (rowStart = 0; rowStart + rowBlock < nRow; rowStart += rowBlock) {
    forest->PredictAcross(predictLeaves, rowStart, rowStart + rowBlock, bag);
    Score(forest, votes, rowStart, rowStart + rowBlock);
    if (prob != 0)
      Prob(forest, prob, rowStart, rowStart + rowBlock);
  }
  if (rowStart < nRow) {
    forest->PredictAcross(predictLeaves, rowStart, nRow, bag);
    Score(forest, votes, rowStart, nRow);
    if (prob != 0)
      Prob(forest, prob, rowStart, nRow);
  }
  Vote(votes, census, &yPred[0]);
  delete [] votes;

  if (yTest != 0) {
    Validate(yTest, &yPred[0], conf, error);
  }
}


/**
   @brief Fills in confusion matrix and error vector.

   @param yCtg contains the training response.

   @param yPred is the predicted response.

   @param confusion is the confusion matrix.

   @param error outputs the classification errors.

   @return void.
*/
void PredictCtg::Validate(const int yCtg[], const int yPred[], int confusion[], double error[]) {
  for (unsigned int row = 0; row < nRow; row++) {
    confusion[ctgWidth * yCtg[row] + yPred[row]]++;
  }

  // Fills in classification error vector from off-diagonal confusion elements..
  //
  for (unsigned int rsp = 0; rsp < ctgWidth; rsp++) {
    int numWrong = 0;
    int numRight = 0;
    for (unsigned int predicted = 0; predicted < ctgWidth; predicted++) {
      if (predicted != rsp) {  // Mispredictions are off-diagonal.
        numWrong += confusion[ctgWidth * rsp + predicted];
      }
      else {
	numRight = confusion[ctgWidth * rsp + predicted];
      }
    }
    error[rsp] = double(numWrong) / double(numWrong + numRight);
  }
}

 
/**
   @brief Voting for non-bagged prediction.  Rounds jittered scores to category.

   @param predictLeaves are the predicted terminal indices.

   @param yCtg outputs predicted response.

   @return void, with output reference vector.
*/
void PredictCtg::Vote(double *votes, int census[], int yPred[]) {
  unsigned int row;

#pragma omp parallel default(shared) private(row)
  {
#pragma omp for schedule(dynamic, 1)
  for (row = 0; row < nRow; row++) {
    int argMax = -1;
    double scoreMax = 0.0;
    double *score = votes + row * ctgWidth;
    for (unsigned int ctg = 0; ctg < ctgWidth; ctg++) {
      double ctgScore = score[ctg]; // Jittered vote count.
      if (ctgScore > scoreMax) {
	scoreMax = ctgScore;
	argMax = ctg;
      }
      census[row * ctgWidth + ctg] = ctgScore; // De-jittered.
    }
    yPred[row] = argMax;
  }
  }
}


/**
   @brief Computes score from leaf predictions.

   @param forest holds the trained forest.

   @return internal vote table, with output reference vector.
 */
void PredictCtg::Score(const Forest *forest, double *votes, unsigned int rowStart, unsigned int rowEnd) {
  unsigned int row;

  // TODO:  Recast loop by blocks, to avoid
  // false sharing.
#pragma omp parallel default(shared) private(row)
  {
#pragma omp for schedule(dynamic, 1)
  for (row = rowStart; row < rowEnd; row++) {
    int *leaves = predictLeaves + (row - rowStart) * nTree;
    double *prediction = votes + row * ctgWidth;
    for (int tc = 0; tc < nTree; tc++) {
      int leafIdx = leaves[tc];
      if (leafIdx >= 0) {
	double val = forest->LeafVal(tc, leafIdx);
	unsigned int ctg = val; // Truncates jittered score for indexing.
	prediction[ctg] += 1 + val - ctg;
      }
    }
  }
  }
}


void PredictCtg::Prob(const Forest *forest, double *prob, unsigned int rowStart, unsigned int rowEnd) {
  for (unsigned int row = rowStart; row < rowEnd; row++) {
    int *leafRow = predictLeaves + (row - rowStart) * nTree;
    double *probRow = prob + row * ctgWidth;
    double rowSum = 0.0;
    for (int tc = 0; tc < nTree; tc++) {
      int leafIdx = leafRow[tc];
      if (leafIdx >= 0) {
        double *idxWeight = leafWeight + ctgWidth * forest->LeafPos(tc, leafIdx);
	for (unsigned int ctg = 0; ctg < ctgWidth; ctg++) {
	  probRow[ctg] += idxWeight[ctg];
	  rowSum += idxWeight[ctg];
	}
      }
    }
    double recipSum = 1.0 / rowSum;
    for (unsigned int ctg = 0; ctg < ctgWidth; ctg++)
      probRow[ctg] *= recipSum;
  }
}


/**
 */
void PredictReg::PredictAcross(const Forest *forest, std::vector<double> &yPred, const BitMatrix *bag) {
  unsigned int rowStart;
  for (rowStart = 0; rowStart + rowBlock < nRow; rowStart += rowBlock) {
    forest->PredictAcross(predictLeaves, rowStart, rowStart + rowBlock, bag);
    Score(forest, &yPred[0], rowStart, rowStart + rowBlock);
  }
  if (rowStart < nRow) {
    forest->PredictAcross(predictLeaves, rowStart, nRow, bag);
    Score(forest, &yPred[0], rowStart, nRow);
  }
}


/**
 */
void PredictReg::PredictAcross(const Forest *forest, std::vector<double> &yPred, Quant *quant, double qPred[], const BitMatrix *bag) {
  unsigned int rowStart;
  for (rowStart = 0; rowStart + rowBlock < nRow; rowStart += rowBlock) {
    forest->PredictAcross(predictLeaves, rowStart, rowStart + rowBlock, bag);
    Score(forest, &yPred[0], rowStart, rowStart + rowBlock);
    quant->PredictAcross(&predictLeaves[rowStart], rowStart, rowStart + rowBlock, qPred);
  }
  if (rowStart < nRow) {
    forest->PredictAcross(predictLeaves, rowStart, nRow, bag);
    Score(forest, &yPred[0], rowStart, nRow);
    quant->PredictAcross(&predictLeaves[rowStart], rowStart, nRow, qPred);
  }
}




/**
  @brief Sets regression scores from leaf predictions.

  @param predictLeaves holds the leaf predictions.

  @param yPred outputs the score predictions.

  @return void, with output refererence vector.
 */
void PredictReg::Score(const Forest *forest, double yPred[], unsigned int rowStart, unsigned int rowEnd) {
  unsigned int row;

#pragma omp parallel default(shared) private(row)
  {
#pragma omp for schedule(dynamic, 1)
  for (row = rowStart; row < rowEnd; row++) {
    double score = 0.0;
    int treesSeen = 0;
    int *leaves = predictLeaves + (row - rowStart) * nTree;
    for (int tc = 0; tc < nTree; tc++) {
      int leafIdx = leaves[tc];
      if (leafIdx >= 0) {
        treesSeen++;
        score += forest->LeafVal(tc, leafIdx);
      }
    }
    yPred[row] = score / treesSeen; // Assumes >= 1 tree seen.
  }
  }
}
