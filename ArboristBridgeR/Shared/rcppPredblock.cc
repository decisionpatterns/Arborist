// Copyright (C)  2012-2016   Mark Seligman
//
// This file is part of ArboristBridgeR.
//
// ArboristBridgeR is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// ArboristBridgeR is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ArboristBridgeR.  If not, see <http://www.gnu.org/licenses/>.

/**
   @file rcppPredBlock.cc

   @brief C++ interface to R entries for maintaining predictor data structures.

   @author Mark Seligman
*/

// Testing only:
//#include <iostream>

#include "rcppPredblock.h"
#include "rowrank.h"


/**
  @brief Extracts contents of a data frame into numeric and (zero-based) factor blocks.  Can be quite slow for large predictor counts, as a linked list is being walked.

  @param sX is the raw data frame, with columns assumed to be either factor or numeric.

  @param sNRow is the number of rows.

  @param sNCol is the number of columns.

  @param sFacCol is the number of factor-valued columns.

  @param sNumCol is the number of numeric-valued columns.

  @param sLevel is a vector of level counts for each column.

  @return PredBlock with separate numeric and integer matrices.
*/
RcppExport SEXP RcppPredBlockFrame(SEXP sX, SEXP sNumElt, SEXP sFacElt, SEXP sLevels, SEXP sSigTrain) {
  DataFrame xf(sX);
  IntegerVector numElt = IntegerVector(sNumElt) - 1;
  IntegerVector facElt = IntegerVector(sFacElt) - 1;
  std::vector<unsigned int> levels = as<std::vector<unsigned int> >(sLevels);
  unsigned int nRow = xf.nrows();
  unsigned int nPredFac = facElt.length();
  unsigned int nPredNum = numElt.length();
  unsigned int nPred = nPredFac + nPredNum;

  IntegerVector predMap(nPred);
  IntegerVector facCard(0);
  IntegerMatrix xFac;
  NumericMatrix xNum;
  if (nPredNum > 0) {
    xNum = NumericMatrix(nRow, nPredNum);
  }
  else
    xNum = NumericMatrix(0, 0);
  if (nPredFac > 0) {
    facCard = IntegerVector(nPredFac);
    xFac = IntegerMatrix(nRow, nPredFac);
  }
  else {
    xFac = IntegerMatrix(0);
  }

  int numIdx = 0;
  int facIdx = 0;
  List level(nPredFac);
  for (unsigned int feIdx = 0; feIdx < nPred; feIdx++) {
    unsigned int card = levels[feIdx];
    if (card == 0) {
      xNum(_, numIdx) = as<NumericVector>(xf[feIdx]);
      predMap[numIdx++] = feIdx;
    }
    else {
      facCard[facIdx] = card;
      level[facIdx] = as<CharacterVector>(as<IntegerVector>(xf[feIdx]).attr("levels"));
      xFac(_, facIdx) = as<IntegerVector>(xf[feIdx]) - 1;
      predMap[nPredNum + facIdx++] = feIdx;
    }
  }

  // Factor positions must match those from training and values must conform.
  //
  if (!Rf_isNull(sSigTrain) && nPredFac > 0) {
    List sigTrain(sSigTrain);
    IntegerVector predTrain(as<IntegerVector>(sigTrain["predMap"]));
    if (!is_true(all( predMap == predTrain)))
      stop("Signature mismatch");

    List levelTrain(as<List>(sigTrain["level"]));
    RcppPredblock::FactorRemap(xFac, level, levelTrain);
  }
  List signature = List::create(
        _["predMap"] = predMap,
        _["level"] = level
	);
  signature.attr("class") = "Signature";
  
  List predBlock = List::create(
      _["colNames"] = colnames(xf),
      _["rowNames"] = rownames(xf),
      _["blockNum"] = xNum,
      _["nPredNum"] = nPredNum,
      _["blockFac"] = xFac,
      _["nPredFac"] = nPredFac,
      _["nRow"] = nRow,
      _["facCard"] = facCard,
      _["signature"] = signature
      );
  predBlock.attr("class") = "PredBlock";

  return predBlock;
}


void RcppPredblock::FactorRemap(IntegerMatrix &xFac, List &levelTest, List &levelTrain) {
  for (int col = 0; col < xFac.ncol(); col++) {
    CharacterVector colTest(as<CharacterVector>(levelTest[col]));
    CharacterVector colTrain(as<CharacterVector>(levelTrain[col]));
    if (is_true(any(colTest != colTrain))) {
      IntegerVector colMatch = match(colTest, colTrain);
      IntegerVector sq = seq(0, colTest.length() - 1);
      IntegerVector idxNonMatch = sq[is_na(colMatch)];
      if (idxNonMatch.length() > 0) {
	warning("Factor levels not observed in training:  employing proxy");
	int proxy = colTrain.length() + 1;
	colMatch[idxNonMatch] = proxy;
      }

      colMatch = colMatch - 1;  // match() is one-based.
      IntegerMatrix::Column xCol = xFac(_, col);
      IntegerVector colT(xCol);
      IntegerVector colRemap = colMatch[colT];
      xFac(_, col) = colRemap;
    }
  }
}


RcppExport SEXP RcppPredBlockNum(SEXP sX) {
  NumericMatrix blockNum(as<NumericMatrix>(sX));
  int nPred = blockNum.ncol();
  List dimnames = blockNum.attr("dimnames");
  List signature = List::create(
      _["predMap"] = seq_len(nPred) - 1,
      _["level"] = List::create(0)
  );
  signature.attr("class") = "Signature";

  IntegerVector facCard(0);
  List predBlock = List::create(
	_["colNames"] = colnames(blockNum),
	_["rowNames"] = rownames(blockNum),
	_["blockNum"] = blockNum,
	_["nPredNum"] = nPred,
        _["blockFac"] = IntegerMatrix(0),
	_["nPredFac"] = 0,
	_["nRow"] = blockNum.nrow(),
        _["facCard"] = facCard,
	_["signature"] = signature
      );
  predBlock.attr("class") = "PredBlock";

  return predBlock;
}


/**
   @brief Unwraps field values useful for prediction.
 */
void RcppPredblock::Unwrap(SEXP sPredBlock, unsigned int &_nRow, unsigned int &_nPredNum, unsigned int &_nPredFac, NumericMatrix &_blockNum, IntegerMatrix &_blockFac) {
  List predBlock(sPredBlock);
  if (!predBlock.inherits("PredBlock"))
    stop("Expecting PredBlock");
  
  _nRow = as<unsigned int>((SEXP) predBlock["nRow"]);
  _nPredFac = as<unsigned int>((SEXP) predBlock["nPredFac"]);
  _nPredNum = as<unsigned int>((SEXP) predBlock["nPredNum"]);
  _blockNum = as<NumericMatrix>((SEXP) predBlock["blockNum"]);
  _blockFac = as<IntegerMatrix>((SEXP) predBlock["blockFac"]);
}


/**
   @brief Unwraps field values useful for export.
 */
void RcppPredblock::SignatureUnwrap(SEXP sSignature, IntegerVector &_predMap, List &_level) {
  List signature(sSignature);
  if (!signature.inherits("Signature"))
    stop("Expecting Signature");
  _predMap = as<IntegerVector>((SEXP) signature["predMap"]);
  _level = as<List>(signature["level"]);
}
