/**
 * @author: Angela N Grigoroaia
 * @date: 21.06.2007
 * @file: simple_data.h
 *
 * @description:
 * Read, store and manipulate coordinates and weights.
 * The main class is called 'DataPack' and it will contain a matrix that stores
 * all the relevant information about the given points (including their weights
 * if any are given). The first 'dimension' features represent the coordinates
 * and the following 'nweights' features are the weights of a particular point.
 *
 * Note:
 * Although everything is stored in a single matrix, the actual representation 
 * is abstracted by aliasing the coordinates and weights as two separate
 * matrices.
 */

#ifndef SIMPLE_DATA_H
#define SIMPLE_DATA_H

#include "fastlib/fastlib.h"
#include "globals.h"

class DataPack {
	public:
		Matrix data;
		int nweights;
		int dimension;

	public:
		DataPack() {}
		~DataPack() {}

	public:
		/* Create an empty datapack. */
		void Init();

		/* Read the data from a file. The number of weights is assumed to be 0. */
		success_t InitFromFile(const char *file);

		/* Read the data from a file and specify the number of weights. */
		success_t InitFromFile(const char *file, const int weights);

		/* Modify the number of columns that are seen as weights. */
		void SetWeights(const int weights);

	/**
	 * Obtain aliases for the coordinates and weights. This is the only method in
	 * which this class should interact with other parts of the code.
	 */
	public: 
		Matrix GetCoordinates();
		Matrix GetWeights();
}

#endif
