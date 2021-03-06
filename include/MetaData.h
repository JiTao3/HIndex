#pragma once
// #include "Point.h"
#include <vector>
#include <array>
#include <torch/torch.h>
// #include <torch/utils.h>
# define INSERT_BUFFERSIZE 1000
# define BITMAP_SIZE 100000

using namespace std;

class MetaData
{
private:
public:
    array<double, 2>* data;
    double map_val;
    static const int dim = 2;

    MetaData();
    MetaData(array<double, 2>* data);
    void setMapVal(double map_val);
    void setMapVal(vector<double>& cellBound, double cellArea);
    void print();
    ~MetaData();
};


