"""
Weather4 Dataset (cloudy, rain, shine, sunrise)
"""
import os, sys
import torch
from torch.utils.data import Dataset
from torchvision import transforms
import albumentations as album
import cv2
import ai8x

class Weather4(Dataset):
    labels = ['cloudy', 'rain', 'shine', 'sunrise']
    label_to_id_map = {k: v for v, k in enumerate(labels)}
    label_to_folder_map = {c: c for c in labels}

    def __init__(self, root_dir, d_type, transform=None,
                 resize_size=(32, 32), augment_data=False):
        # expects: <root_dir>/weather4/<train|test>/<class>/*
        self.root_dir = root_dir
        self.data_dir = os.path.join(root_dir, 'weather4', d_type)

        if not os.path.isdir(self.data_dir):
            self.__print_manual()
            sys.exit("Dataset not found!")

        self._scan()

        if d_type == 'train' and augment_data:
            self.album_transform = album.Compose([
                album.GaussNoise(var_limit=(1.0, 20.0), p=0.25),
                album.RGBShift(r_shift_limit=15, g_shift_limit=15, b_shift_limit=15, p=0.5),
                album.ColorJitter(p=0.5),
                album.SmallestMaxSize(max_size=int(1.2*min(resize_size))),
                album.ShiftScaleRotate(shift_limit=0.05, scale_limit=0.05, rotate_limit=15, p=0.5),
                album.RandomCrop(height=32, width=32),
                album.HorizontalFlip(p=0.5),
                album.Normalize(mean=(0.0, 0.0, 0.0), std=(1.0, 1.0, 1.0))])
        else:
            self.album_transform = album.Compose([
                album.SmallestMaxSize(max_size=int(1.2*min(resize_size))),
                album.CenterCrop(height=32, width=32),
                album.Normalize(mean=(0.0, 0.0, 0.0), std=(1.0, 1.0, 1.0))])

        self.transform = transform

    def _scan(self):
        self.data_list = []
        for label in self.labels:
            d = os.path.join(self.data_dir, self.label_to_folder_map[label])
            if not os.path.isdir(d):
                continue
            for fn in sorted(os.listdir(d)):
                fp = os.path.join(d, fn)
                if os.path.isfile(fp):
                    self.data_list.append((fp, self.label_to_id_map[label]))

    def __len__(self): return len(self.data_list)

    def __getitem__(self, index):
        label = torch.tensor(self.data_list[index][1], dtype=torch.int64)
        fp = self.data_list[index][0]
        img = cv2.imread(fp)
        if img is None:
            return self.__getitem__((index + 1) % len(self.data_list))
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        img = self.album_transform(image=img)["image"]
        if self.transform: img = self.transform(img)
        return img, label

    def __print_manual(self):
        print("******************************************")
        print("Place images like:")
        print("  'data/weather4/train/cloudy|rain|shine|sunrise/*'")
        print("  'data/weather4/test/cloudy|rain|shine|sunrise/*'")
        print("******************************************")

def get_weather4_dataset(data, load_train, load_test):
    (data_dir, args) = data
    transform = transforms.Compose([
        transforms.ToTensor(),
        ai8x.normalize(args=args),
    ])
    train_ds = Weather4(root_dir=data_dir, d_type='train',
                        transform=transform, augment_data=True) if load_train else None
    test_ds  = Weather4(root_dir=data_dir, d_type='test',
                        transform=transform, augment_data=False) if load_test else None
    return train_ds, test_ds

datasets = [{
    'name': 'weather4',
    'input': (3, 32, 32),
    'output': ('cloudy','rain','shine','sunrise'),
    'loader': get_weather4_dataset,
}]
