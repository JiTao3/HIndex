#include "Utils.h"
#include <algorithm>

using namespace std;

bool equalMetadata(MetaData &me_data1, MetaData &me_data2)
{
	if (me_data1.map_val != me_data2.map_val)
	{
		return false;
	}
	else
	{
		for (int i = 0; i < MetaData::dim; i++)
		{
			if (me_data1.data[i] != me_data2.data[i])
			{
				return false;
			}
		}
	}
	return true;
}

double percentile(vector<double> &vectorIn, double percent, bool is_sorted)
{
	if (!is_sorted)
		sort(vectorIn.begin(), vectorIn.end());
	double res = 0.0;
	double x = (vectorIn.size() - 1) * percent;
	int i = (int)x;
	double j = i - x;
	if (i == (vectorIn.size() - 1))
	{
		return vectorIn[i];
	}
	res = (1 - j) * vectorIn[i] + j * vectorIn[i + 1];
	return res;
}

// template std::vector<double> LinSpace(double start_in, double end_in, int num_in);

vector<int> idx_to_vector_int(int idx, int dim)
{
	vector<int> vector_int;
	for (int i = 0; i < dim; i++)
	{
		if (idx % 2 == 0)
		{
			vector_int.push_back(0);
		}
		else
		{
			vector_int.push_back(1);
		}
		idx /= 2;
	}
	return vector_int;
}

bool match_range(array<double, 2> point_data, vector<double> cell_bound, int dim)
{
	for (int i = 0; i < dim; i++)
	{
		if (point_data[i] < cell_bound[i * 2] || point_data[i] >= cell_bound[i * 2 + 1])
		{
			return false;
		}
	}
	return true;
}

bool match_region(const MetaData &medata, vector<double> split_point, vector<int> left_or_right)
{
	bool match_flag = true;
	for (int i = 0; i < MetaData::dim; i++)
	{
		if (left_or_right[i] == 0)
		{
			match_flag = (match_flag && ((*medata.data)[i] <= split_point[i]));
		}
		else if (left_or_right[i] == 1)
		{
			match_flag = (match_flag && ((*medata.data)[i] > split_point[i]));
		}
		if (!match_flag)
		{
			return false;
		}
	}
	return true;
}

vector<MetaData> splitLeafNodeToLeaf(vector<MetaData> &metadatas, vector<double> split_point, int index)
{
	vector<int> data_left_or_right = idx_to_vector_int(index, split_point.size());
	vector<MetaData> medataVec;
	for (auto &medata : metadatas)
	{
		if (match_region(medata, split_point, data_left_or_right))
		{
			medataVec.push_back(medata);
		}
	}
	return medataVec;
}

vector<MetaData> splitLeafNodeToGrid(vector<MetaData> &metadatas, vector<double> range_bound)
{
	vector<MetaData> metadataVec;
	for (auto &medata : metadatas)
	{
		if (match_range(*medata.data, range_bound, MetaData::dim))
		{
			metadataVec.push_back(medata);
		}
	}
	return metadataVec;
}

vector<double> splitNodeToSplitedRegion(vector<double> cell_range_bound, vector<double> split_point, int index)
{
	vector<int> data_left_or_right = idx_to_vector_int(index, split_point.size());
	vector<double> split_region;
	for (int i = 0; i < MetaData::dim; i++)
	{
		double min_b = cell_range_bound[i * 2];
		double max_b = cell_range_bound[i * 2 + 1];
		if (data_left_or_right[i] == 0)
		{
			split_region.push_back(min_b);
			split_region.push_back(split_point[i]);
		}
		else
		{
			split_region.push_back(split_point[i]);
			split_region.push_back(max_b);
		}
	}
	return split_region;
}

vector<double> splitLeafNodeToGridRegion(vector<double> cell_range_bound, vector<double> split_point, int split_dim, int child_index)
{
	vector<double> split_region;
	for (int i = 0; i < MetaData::dim; i++)
	{
		if (i == split_dim)
		{
			split_region.push_back(split_point[child_index]);
			split_region.push_back(split_point[child_index + 1]);
		}
		else
		{
			split_region.push_back(cell_range_bound[i * 2]);
			split_region.push_back(cell_range_bound[i * 2 + 1]);
		}
	}
	return split_region;
}

vector<array<double, 2> *> &bindary_search(vector<MetaData> &metadataVec, int begin_idx, int end_idx, MetaData &meta_key, std::vector<array<double, 2> *> &result, ExpRecorder &expr)
{
	// auto start_bin = chrono::high_resolution_clock::now();

	int start = begin_idx;
	int end = end_idx;
	int mid = -1;
	while (start <= end)
	{
		mid = (start + end) / 2;
		if (equalMetadata(metadataVec[mid], meta_key))
		{
			result.push_back(metadataVec[mid].data);
			break;
		}
		else if (meta_key.map_val < metadataVec[mid].map_val)
		{
			end = mid - 1;
		}
		else
		{
			start = mid + 1;
		}
	}

	int left = mid - 1;
	int right = mid + 1;
	while (left >= begin_idx && metadataVec[left].map_val == meta_key.map_val)
	{
		if (equalMetadata(metadataVec[left], meta_key))
		{
			result.push_back(metadataVec[left].data);
		}
		left--;
	}
	while (right <= end_idx && metadataVec[right].map_val == meta_key.map_val)
	{
		if (equalMetadata(metadataVec[right], meta_key))
		{
			result.push_back(metadataVec[right].data);
		}
		right++;
	}
	// auto end_bin = chrono::high_resolution_clock::now();
	// expr.pointBindarySearchTime +=chrono::duration_cast<chrono::nanoseconds>(end_bin - start_bin).count();

	return result;
}

int bindarySearchAdjustPrePos(vector<MetaData> &metadataVec, int beginIndex, int endIndex, MetaData &meta_key, int LeftORRight)
{
	// leftORRight > 0 👉
	// leftORRight < 0 👈
	// int adjustPrePos = -1;
	int begin = beginIndex;
	int end = endIndex;
	int mid = -1;
	while (begin < end)
	{
		mid = (begin + end) / 2;
		if (metadataVec[mid].map_val == meta_key.map_val)
			break;
		else if (meta_key.map_val < metadataVec[mid].map_val)
			end = mid - 1;
		else
			begin = mid + 1;
	}
	int left = mid - 1;
	int right = mid + 1;
	if (LeftORRight < 0)
	{
		while (left >= beginIndex && metadataVec[left].map_val == meta_key.map_val)
		{
			left--;
		}
		return left;
	}
	else
	{
		while (right <= endIndex && metadataVec[right].map_val == meta_key.map_val)
		{
			right++;
		}
		return right;
	}
}

int adjustPosition(vector<MetaData> &metadataVec, vector<int> error_bound, int pre_position, MetaData meta_key)
{
	if (metadataVec[pre_position].map_val = meta_key.map_val)
		return pre_position;
	else if (metadataVec[pre_position].map_val > meta_key.map_val)
	{
		// std::vector<int> offsets = bindary_search(metadataVec, pre_position + error_bound[0], pre_position, meta_key);
		// pre_position = *std::max_element(offsets.begin(), offsets.end());
		return bindarySearchAdjustPrePos(metadataVec, pre_position + error_bound[0], pre_position, meta_key, 1);
	}
	else
	{
		// std::vector<int> offsets = bindary_search(metadataVec, pre_position, pre_position + error_bound[0], meta_key);
		// pre_position = *std::min_element(offsets.begin(), offsets.end());
		return bindarySearchAdjustPrePos(metadataVec, pre_position, pre_position + error_bound[1], meta_key, -1);
	}
	// return pre_position;
}

void scan(vector<MetaData> &metadataVec, int begin, int end, double *min_range, double *max_range, vector<array<double, 2> *> &result)
{
	for (int i = begin; i <= end; i++)
	{
		bool is_in = true;
		for (int idx = 0; idx < MetaData::dim; idx++)
		{
			if (!((*metadataVec[i].data)[idx] >= min_range[i] && (*metadataVec[i].data)[idx] <= max_range[i]))
			{
				is_in = false;
				break;
			}
		}
		if (is_in)
		{
			result.push_back(metadataVec[i].data);
		}
	}
}

vector<int> getCellIndex(array<double, 2> &raData, vector<double> &initial_partition_bound_x, vector<double> &initial_partition_bound_y)
{
	vector<int> cell_index;
	for (int j = 0; j < initial_partition_bound_x.size(); j++)
	{
		if (initial_partition_bound_x[j] > raData[0])
		{
			cell_index.push_back(j - 1);
			break;
		}
	}
	for (int j = 0; j < initial_partition_bound_y.size(); j++)
	{
		if (initial_partition_bound_y[j] > raData[1])
		{
			cell_index.push_back(j - 1);
			break;
		}
	}

	return cell_index;
}

bool compareMetadata(MetaData me_data1, MetaData me_data2)
{
	return me_data1.map_val < me_data2.map_val;
}

void orderMetaData(vector<MetaData> &metadataVec)
{
	std::sort(metadataVec.begin(), metadataVec.end(), compareMetadata);
}

int queryCellRealtion(vector<double> &rangeBound, vector<double> &query)
{
	// bool overlapFlag = OVERLAP;

	for (int i = 0; i < MetaData::dim; i++)
	{
		// min(node max and range max)
		float dimMax = rangeBound[2 * i + 1] > query[2 * i + 1] ? query[2 * i + 1] : rangeBound[2 * i + 1];
		// max(node min and range min)
		float dimMin = rangeBound[2 * i] > query[2 * i] ? rangeBound[2 * i] : query[2 * i];
		if (dimMax < dimMin)
			return DISSOCIATION;
	}
	for (int i = 0; i < MetaData::dim; i++)
	{
		if (!(query[i * 2] < rangeBound[i * 2] && query[i * 2 + 1] > rangeBound[i * 2 + 1]))
			return INTERSECTION;
	}
	return CONTAIN;
}

double distFunction(array<double, 2> *point1, array<double, 2> &point2)
{
	return sqrt(pow(((*point1)[0] - point2[0]), 2) + pow(((*point1)[1] - point2[1]), 2));
}