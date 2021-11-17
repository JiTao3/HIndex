
#include "GridNode.h"

using namespace std;

GridNode::GridNode()
{
}

GridNode::GridNode(vector<MetaData> metadataVec, vector<double> rangeBound)
{
	this->metadataVec = metadataVec;
	this->rangeBound = rangeBound;
	mapvalBound[0] = metadataVec[0].map_val;
	mapvalBound[1] = metadataVec[metadataVec.size() - 1].map_val;
	vector<double> scalingMapValVec;
	for (auto &medata : this->metadataVec)
	{
		scalingMapValVec.push_back(this->mapValtoScaling(medata.map_val));
	}
	this->index_model = new IndexModel(scalingMapValVec);
	// before build model you should change the metadata map val to scaling one?
	// this->index_model->buildModel();
}

GridNode::~GridNode()
{
	if (this->index_model != nullptr)
	{
		delete this->index_model;
	}
}

vector<array<double, 2> *>& GridNode::pointSearch(array<double, 2> key, vector<array<double, 2> *>& result, ExpRecorder& exp_Recorder)
{
	// auto start_prePos = chrono::high_resolution_clock::now();
	MetaData meta_key(&key);
	double area = this->getCellArea();
	meta_key.setMapVal(this->parent_rangeBound, area);

	int pre_position = index_model->preFastPosition(this->mapValtoScaling(meta_key.map_val));
	pre_position = pre_position > metadataVec.size() - 1 ? metadataVec.size() - 1 : pre_position;
	pre_position = pre_position > 0 ? pre_position : 0;
	// auto end_prePos = chrono::high_resolution_clock::now();
	// exp_Recorder.pointModelPreTime +=chrono::duration_cast<chrono::nanoseconds>(end_prePos - start_prePos).count();

	// auto start_BinSearch = chrono::high_resolution_clock::now();
	double pre_mapval = metadataVec[pre_position].map_val;
	if (pre_mapval > meta_key.map_val)
	{
		// int min_search_index = std::max(pre_position + index_model->error_bound[1], 0);
		int min_search_index = pre_position + index_model->error_bound[0] > 0 ? pre_position + index_model->error_bound[0] : 0;
		return bindary_search(metadataVec, min_search_index, pre_position, meta_key, result, exp_Recorder);
	}
	else
	{
		int max_search_position = std::min(pre_position + index_model->error_bound[0], (int)(metadataVec.size() - 1));
		return bindary_search(metadataVec, pre_position, max_search_position, meta_key, result, exp_Recorder);
	}
}

vector<array<double, 2> *>& GridNode::rangeSearch(vector<double> query_range, vector<array<double, 2> *> &result)
{
	double *min_range = new double[MetaData::dim];
	double *max_range = new double[MetaData::dim];
	for (int i = 0; i < MetaData::dim; i++)
	{
		min_range[i] = query_range[i * 2];
		max_range[i] = query_range[i * 2 + 1];
	}

	double *node_min = new double[MetaData::dim];
	double *node_max = new double[MetaData::dim];

	for (int i = 0; i < MetaData::dim; i++)
	{
		node_min[i] = rangeBound[i * 2];
		node_max[i] = rangeBound[i * 2 + 1];
	}
	array<double, MetaData::dim> overlap_min;
	array<double, MetaData::dim> overlap_max;
	for (int i = 0; i < MetaData::dim; i++)
	{
		if (node_min[i] > min_range[i] && node_max[i] < max_range[i])
		{
			overlap_min[i] = node_min[i];
			overlap_max[i] = node_max[i];
		}
		else if (node_min[i] < min_range[i] && node_max[i] > max_range[i])
		{
			overlap_min[i] = min_range[i];
			overlap_max[i] = max_range[i];
		}
		else if (node_min[i] > min_range[i])
		{
			overlap_min[i] = node_min[i];
			overlap_max[i] = max_range[i];
		}
		else if (node_max[i] > min_range[i])
		{
			overlap_min[i] = min_range[i];
			overlap_max[i] = node_max[i];
		}
	}

	double cell_area = this->getCellArea();

	MetaData meta_min(&overlap_min);
	meta_min.setMapVal(parent_rangeBound, cell_area);
	MetaData meta_max(&overlap_max);
	meta_max.setMapVal(parent_rangeBound, cell_area);
	int pre_min_position = index_model->preFastPosition(this->mapValtoScaling(meta_min.map_val));
	int pre_max_position = index_model->preFastPosition(this->mapValtoScaling(meta_max.map_val));

	pre_min_position = std::max(pre_min_position, 0);
	pre_max_position = std::max(pre_max_position, 0);

	pre_min_position = std::min(pre_min_position, (int)(metadataVec.size() - 1));
	pre_max_position = std::min(pre_max_position, (int)(metadataVec.size() - 1));

	pre_min_position = adjustPosition(metadataVec, index_model->error_bound, pre_min_position, meta_min);
	pre_max_position = adjustPosition(metadataVec, index_model->error_bound, pre_max_position, meta_max);
	scan(metadataVec, pre_min_position, pre_max_position, min_range, max_range, result);

	delete[] min_range;
	delete[] max_range;
	delete[] node_min;
	delete[] node_max;

	return result;
}

double GridNode::getCellArea()
{
	double area = 1.0;
	for (int i = 0; i < MetaData::dim; i++)
	{
		area *= (parent_rangeBound[i * 2 + 1] - parent_rangeBound[i * 2]);
	}
	return area;
}

double GridNode::getGridArea()
{
	double area = 1.0;
	for (int i = 0; i < MetaData::dim; i++)
	{
		area *= (rangeBound[i * 2 + 1] - rangeBound[i * 2]);
	}
	return area;
}


double GridNode::mapValtoScaling(double mapVal)
{
	return (mapVal - this->mapvalBound[0]) / (this->mapvalBound[1] - this->mapvalBound[0]);
}
double GridNode::scalingtoMapVal(double scaling)
{
	return scaling * (this->mapvalBound[1] - this->mapvalBound[0]) + this->mapvalBound[0];
}

void GridNode::saveMetaDataVectoCSV(string file_path)
{
	std::ofstream outfile(file_path);

	for (auto &medata : this->metadataVec)
	{
		for (auto i : *medata.data)
		{
			outfile << i << ',';
		}
		outfile << medata.map_val << ",";
		outfile << this->mapValtoScaling(medata.map_val);
		outfile << '\n';
	}
}