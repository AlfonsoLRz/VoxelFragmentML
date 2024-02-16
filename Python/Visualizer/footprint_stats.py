import multiprocessing
import numpy as np
from pathlib import Path
from tqdm import tqdm
import zipfile


def calculate_footprint(p_idx, files_t, unit_divisor=1024 ** 2):
    print(f'Process {p_idx} calculating footprint for {len(files_t)} files')

    footprint_compressed_t = .0
    footprint_uncompressed_t = .0
    count_files_t = 0

    for file in files_t:
        if zipfile.is_zipfile(file):
            with zipfile.ZipFile(file, 'r') as zip_ref_t:
                uncompressed_size = 0
                for info in zip_ref_t.infolist():
                    uncompressed_size += info.file_size
                compressed_size = file.stat().st_size

                footprint_compressed_t += compressed_size / unit_divisor
                footprint_uncompressed_t += uncompressed_size / unit_divisor
                count_files_t += 1

    return footprint_compressed_t, footprint_uncompressed_t, count_files_t


if __name__ == '__main__':
    resolution = [64, 128]
    #resolution = [128]
    extensions = [['.bing', '.vox', '.rle', '.qstack'], ['.ply', '.xyz', '.binp'], ['.obj', '.stl', '.binm']]
    headers = [['Binary raw grid', 'MagicaVoxel', 'RLE', 'QuadStack'], ['PLY', 'XYZ', 'Compressed binary'],
               ['OBJ', 'STL', 'Binary']]
    folder = 'D:/allopezr/Fragments/Vessels_200/'

    n_splits = 15
    footprint_compressed = np.zeros((len(resolution), len(extensions), 4))
    footprint_uncompressed = np.zeros((len(resolution), len(extensions), 4))
    count_files = np.zeros((len(resolution), len(extensions), 4))

    # Mb
    unit_divisor = 1024 ** 3

    for res_idx, res in enumerate(resolution):
        for data_type_idx, data_type in enumerate(extensions):
            for extension_idx, extension in enumerate(data_type):
                # Search recursively for files with the given extension
                files = [f for f in Path(folder).rglob('*' + str(res) + 'r*' + extension + '.zip')]
                print(f'Found {len(files)} files with extension {extension}')

                # Split the files into n_threads chunks
                n_splits = min(n_splits, len(files))
                files_chunks = np.array_split(files, n_splits)

                processes = []
                footprint_compressed_list = []
                footprint_uncompressed_list = []
                count_files_list = []

                print(f'Calculating footprint for {len(files)} files with {n_splits} processes')
                with multiprocessing.Pool(processes=n_splits) as pool:
                    matrices = [pool.apply_async(calculate_footprint, (i, files_chunks[i], 1024 ** 2))
                                for i in range(n_splits)]
                    for result in tqdm(matrices):
                        footprint_compressed_r, footprint_uncompressed_r, count_files_r = result.get()

                        footprint_compressed[res_idx, data_type_idx, extension_idx] += footprint_compressed_r
                        footprint_uncompressed[res_idx, data_type_idx, extension_idx] += footprint_uncompressed_r
                        count_files[res_idx, data_type_idx, extension_idx] += count_files_r

                    pool.close()

    average_footprint_compressed = footprint_compressed / count_files
    average_footprint_uncompressed = footprint_uncompressed / count_files

    # latex_table_str = ''
    # for data_type_idx, data_type in enumerate(headers):
    #     for extension_idx, extension in enumerate(data_type):
    #         latex_table_str += '\\textbf{' + headers[data_type_idx][extension_idx] + '} & '
    #
    #         for res_idx, res in enumerate(resolution):
    #             latex_table_str += (f'{footprint_compressed[res_idx, data_type_idx, extension_idx]/1024:.2f}GB '
    #                                 f'({footprint_uncompressed[res_idx, data_type_idx, extension_idx]/1024:.2f}GB) & ')
    #             latex_table_str += (f'{average_footprint_compressed[res_idx, data_type_idx, extension_idx]:.2f}MB '
    #                                 f'({average_footprint_uncompressed[res_idx, data_type_idx, extension_idx]:.2f}MB) '
    #                                 f'& ')
    #
    #         latex_table_str = latex_table_str[:-2] + '\\\\ \n'
    #     latex_table_str = latex_table_str[:-2] + '\n\cmidrule{1-7}\n'
    # latex_table_str = latex_table_str[:-1]

    latex_table_str = ''
    for data_type_idx, data_type in enumerate(headers):
        for extension_idx, extension in enumerate(data_type):
            latex_table_str += '\\textbf{' + headers[data_type_idx][extension_idx] + '} & '

            for res_idx, res in enumerate(resolution):
                latex_table_str += (f'{footprint_compressed[res_idx, data_type_idx, extension_idx]/1024:.2f}GB '
                                    f'({footprint_uncompressed[res_idx, data_type_idx, extension_idx]/1024:.2f}GB) & ')

            latex_table_str = latex_table_str[:-2] + '\\\\ \n'
        latex_table_str = latex_table_str[:-2] + '\n\cmidrule{1-4}\n'
    latex_table_str = latex_table_str[:-1]

    print(latex_table_str)
