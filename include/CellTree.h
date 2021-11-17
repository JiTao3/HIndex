#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <array>
#include <cmath>
#include <chrono>

#include <boost/variant.hpp>
#include <boost/variant/get.hpp>
// #include <boost/asio/thread_pool.hpp>
#include <torch/torch.h>

#include "Utils.h"
#include "MetaData.h"
#include "FileReader.h"
#include "IndexModel.h"
#include "GridNode.h"
#include "LeafNode.h"
#include "InnerNode.h"
#include "ExpRecorder.h"
// #include "ThreadPool.h"
// #include "Point.h"

void getAllMetaData(boost::variant<InnerNode *, LeafNode *, GridNode *, int> root, vector<MetaData> &all_medata);
void getAllData(boost::variant<InnerNode *, LeafNode *, GridNode *, int> root, vector<array<double, 2> *> &result);

class CellTree
{
public:
	vector<array<double, 2>> raw_data;
	vector<vector<vector<MetaData>>> initial_partition_data;
	int split_num = 100;
	std::vector<double> init_partion_bound_x;
	std::vector<double> init_partion_bound_y;
	std::vector<double> data_space_bound; // initial root node
	std::vector<std::vector<int>> cell_bound_idx;

	InnerNode root;

	int cellSplitTh = 50000;
	int gridSplitTh = 15000;
	int mergeTh = 5000;

	int modelCapability = 10000;

	string saveSplitPath;
	string modelParamPath;

	// hist info for knn

	vector<double> hist_info_x;
	vector<double> hist_info_y;
	int bin_num = 500;

public:
	CellTree();
	~CellTree();
	CellTree(int split_num, vector<double> data_space_bound, string raw_path);
	void loadRawData(string file_path);
	void loadSampleData(string file_path);
	void initialPartionBound(int n);
	void initialPartionData();
	void buildTree(std::vector<std::vector<int>> cell_bound_idx, InnerNode *root_node, int child_idx);
	void saveSplitData(boost::variant<InnerNode *, LeafNode *, GridNode *, int> root);
	void buildCheck(boost::variant<InnerNode *, LeafNode *, GridNode *, int> root, int child_index);
	vector<array<double, 2> *> &pointSearch(array<double, 2> &query, vector<array<double, 2> *> &result, ExpRecorder &exp_Recorder);
	vector<array<double, 2> *> &rangeSearch(vector<double> &query, vector<array<double, 2> *> &result, ExpRecorder &exp_Recorder);
	vector<array<double, 2> *> &kNNSearch(vector<double> &query, int k, vector<array<double, 2> *> &result);
	void DFSCelltree(vector<double> &query, vector<array<double, 2> *> &result, boost::variant<InnerNode *, LeafNode *, GridNode *, int> root);
	// vector<vector<double> *> pointTravel(vector<double> &query, boost::variant<InnerNode *, LeafNode *, GridNode *, int> root);
	void train(boost::variant<InnerNode *, LeafNode *, GridNode *, int> root);
};