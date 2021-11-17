import os
import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader, dataloader

import numpy as np

from torch.multiprocessing import Pool, Process, set_start_method

device = torch.device("cuda:4" if torch.cuda.is_available() else "cpu")


class IndexDataSet(Dataset):
    def __init__(self, data) -> None:
        super().__init__()
        self.data = data
        self.length = float(len(self.data))

    def __len__(self) -> int:
        return len(self.data)

    def __getitem__(self, index):
        if torch.is_tensor(index):
            index = index.tolist()
        sample = {
            "map_val": torch.FloatTensor([self.data[index]]),
            "position": torch.FloatTensor([index]) / (self.length - 1),
        }
        return sample


class NNRegressionModel(nn.Module):
    def __init__(self, input_s, hiden_s, output_s=1) -> None:
        super().__init__()
        self.input = nn.Linear(input_s, hiden_s)
        self.ac_f1 = nn.ReLU()
        self.hiden = nn.Linear(hiden_s, output_s)
        self.ac_f2 = nn.ReLU()

    def init_weight(self, weight):
        self.input.weight.data = weight[:100].view(-1, 1)
        self.input.bias.data = weight[100:200].view(-1)
        self.hiden.weight.data = weight[200:300].view(1, -1)
        self.hiden.bias.data = weight[300:301].view(-1)

    def forward(self, x):
        x = self.input(x)
        x = self.ac_f1(x)
        x = self.hiden(x)
        x = self.ac_f2(x)
        return x

    def model_weight_all(self):
        weight = self.input.weight.data.view(-1)
        weight = torch.cat((weight, self.input.bias.data.view(-1)))
        weight = torch.cat((weight, self.hiden.weight.data.view(-1)))
        weight = torch.cat((weight, self.hiden.bias.data.view(-1)))
        return weight.cpu().numpy()


def trainMetaParam(model_param_path, raw_data_path, save_path, index):
    print("index: ", index)
    model_weight = np.genfromtxt(model_param_path, delimiter=",")
    model_weight = torch.FloatTensor(model_weight, requires_grad=True)
    regression_model = NNRegressionModel(1, 100, 1)
    regression_model.init_weight(model_weight)
    regression_model.to(device)

    raw_data = np.genfromtxt(raw_data_path, delimiter=",")
    train_data = IndexDataSet(raw_data[:, -1])
    train_data_loader = DataLoader(
        train_data, batch_size=raw_data.shape[0], shuffle=True, pin_memory=True
    )
    lossf = nn.MSELoss()
    optimizer = torch.optim.Adam(regression_model.parameters(), lr=0.01)
    num_epoch = 200
    for epoch in range(num_epoch):
        for sample in train_data_loader:
            x = sample["map_val"].to(device)
            y = sample["position"].to(device)
            optimizer.zero_grad()
            y_p = regression_model(x)
            loss = lossf(y_p, y)
            loss.backward()
            optimizer.step()
    np_weight = regression_model.model_weight_all()
    np.savetxt(save_path, np_weight, delimiter=",")
    print("finish: ", index)


def trainRandomInitialModle(raw_data_path, save_path, index):
    regression_model = NNRegressionModel(1, 100, 1)
    # regression_model.init_weight(model_weight)
    regression_model.to(device)

    raw_data = np.genfromtxt(raw_data_path, delimiter=",")
    train_data = IndexDataSet(raw_data[:, -1])
    train_data_loader = DataLoader(
        train_data, batch_size=raw_data.shape[0], shuffle=True, pin_memory=True
    )
    lossf = nn.MSELoss()
    optimizer = torch.optim.Adam(regression_model.parameters(), lr=0.01)
    num_epoch = 200
    for epoch in range(num_epoch):
        # mse_loss = 0
        for sample in train_data_loader:
            x = sample["map_val"].to(device)
            y = sample["position"].to(device)
            optimizer.zero_grad()
            y_p = regression_model(x)
            loss = lossf(y_p, y)
            # mse_loss += loss.item()
            loss.backward()
            optimizer.step()
        # print("epoch:{}, mse loss {:.6}".format(epoch, mse_loss))
    np_weight = regression_model.model_weight_all()
    np.savetxt(save_path, np_weight, delimiter=",")
    print("finish: ", index)


def model_error(raw_data_path, model_param_path):
    regression_model = NNRegressionModel(1, 100, 1)
    model_weight = np.genfromtxt(model_param_path, delimiter=",")
    model_weight = torch.FloatTensor(model_weight)
    regression_model.init_weight(model_weight)
    raw_data = np.genfromtxt(raw_data_path, delimiter=",")
    train_data = IndexDataSet(raw_data[:, -1])
    train_data_loader = DataLoader(train_data, batch_size=raw_data.shape[0])
    torch.no_grad()
    lower_error = 0
    upper_error = 0
    for sample in train_data_loader:
        x = sample["map_val"]
        y = sample["position"]
        y_p = regression_model(x)
        for y_g, ypre in zip(y, y_p):
            error = (y_g - ypre)*raw_data.shape[0]
            if error>0 and error>upper_error:
                upper_error = error
            elif error <0 and error<lower_error:
                lower_error = error
    print(lower_error, upper_error)
    return lower_error, upper_error


if __name__ == "__main__":
    model_param_path = "/data/jitao/dataset/OSM/trained_modelParam_for_split2_largeBatch/1385.csv"
    raw_data_path = "/data/jitao/dataset/OSM/split2/1385.csv"
    # trainRandomInitialModle(raw_data_path, model_param_path, 2)
    model_error(raw_data_path, model_param_path)


# if __name__ == "__main__":
#     torch.multiprocessing.set_start_method("spawn")
#     model_param_path = (
#         "/data/jitao/dataset/OSM/new_init_model_parameter_for_split_2/"
#     )
#     raw_data_path = "/data/jitao/dataset/OSM/split2/"
#     save_path = "/data/jitao/dataset/OSM/trained_modelParam_for_split2_largeBatch/"
#     # save_random_path = (
#     #     "/data/jitao/dataset/OSM/random_trained_model_param_for_split2/"
#     # )
#     data_name_list = os.listdir(raw_data_path)
#     training_pool = Pool(20)
#     for index, data_name in enumerate(data_name_list):
#         training_pool.apply_async(
#             # trainRandomInitialModle,
#             # (
#             #     raw_data_path + data_name,
#             #     save_random_path + data_name,
#             #     index,
#             # ),
#             trainMetaParam,
#             (
#                 model_param_path + data_name,
#                 raw_data_path + data_name,
#                 save_path + data_name,
#                 index
#             )
#         )
#     training_pool.close()
#     training_pool.join()


# nohup python train.py > ../log/training_osm_cn_split2_largeBatch.log 2>&1 &