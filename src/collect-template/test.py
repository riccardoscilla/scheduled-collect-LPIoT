import os
import glob
import pandas as pd
import numpy as np
from collections import defaultdict
import matplotlib.pyplot as plt

n_test = 1
# folder = "test_udgm_contikiMAC/"
# folder = "test_udgm_nullRDC/"

# folder = "test_udgm_contikiMAC_rx20/"
# folder = "test_udgm_nullRDC_rx20/"

folder = "random"

def plot_pdr():
    df = pd.read_csv(folder+'pdr.csv', delimiter="\t")
    pdr = defaultdict(int)
    for index, row in df.iterrows():
        pdr[row['node']] += row['pdr']

    labels = []
    values = []

    for n in pdr:
        pdr[n] /= n_test
        labels.append('node '+str(int(n)))
        values.append(pdr[n])

    x = np.arange(len(labels)) 
    fig, ax = plt.subplots()
    rects = ax.bar(x, values)
    ax.set_ylabel('Average PDR (%)')
    ax.set_title('PDR')
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()

    plt.show()

def plot_dc():
    df = pd.read_csv(folder+'/dc.csv', delimiter="\t")
    pdr = defaultdict(int)
    for index, row in df.iterrows():
        pdr[row['node']] += row['dc']

    labels = []
    values = []

    for n in pdr:
        pdr[n] /= n_test
        labels.append('node '+str(int(n)))
        values.append(pdr[n])

    x = np.arange(len(labels)) 
    fig, ax = plt.subplots()
    rects = ax.bar(x, values)
    ax.set_ylabel('Average Duty Cycle (%)')
    ax.set_title('Duty Cycle')
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()

    plt.show()

def run_udgm():
    for i in range(n_test):
        os.system("cooja_nogui test_nogui_udgm.csc")
        os.system("python parse-stats.py test_nogui_udgm.log")

        # move pdr csv
        os.rename("test_nogui_udgm-pdr.csv", folder+str(i)+"-pdr.csv")

        # move dc csv
        os.rename("test_nogui_udgm-dc.csv", folder+str(i)+"-dc.csv")

        # remove log files
        os.remove("test_nogui_udgm.log")
        os.remove("test_nogui_udgm_dc.log")

        # remove other csv files
        os.remove("test_nogui_udgm-energest.csv")
        os.remove("test_nogui_udgm-recv.csv")
        os.remove("test_nogui_udgm-sent.csv")
    
    # combine all pdr csv
    all_filenames = [i for i in glob.glob(folder+'*-pdr.csv')]
    combined_csv = pd.concat([pd.read_csv(f) for f in all_filenames ])
    combined_csv.to_csv( folder+"/pdr.csv", index=False, encoding='utf-8-sig')

    for fname in all_filenames:
        os.remove(fname)

    # combine all dc csv
    all_filenames = [i for i in glob.glob(folder+'*-dc.csv')]
    combined_csv = pd.concat([pd.read_csv(f) for f in all_filenames ])
    combined_csv.to_csv( folder+"/dc.csv", index=False, encoding='utf-8-sig')

    for fname in all_filenames:
        os.remove(fname)

def run_mrm():
    for i in range(n_test):
        os.system("cooja_nogui test_nogui_mrm_line.csc")
        os.system("python parse-stats.py test_nogui_mrm_line.log")

        # move pdr csv
        os.rename("test_nogui_mrm_line-pdr.csv", folder+str(i)+"-pdr.csv")

        # move dc csv
        os.rename("test_nogui_mrm_line-dc.csv", folder+str(i)+"-dc.csv")

        # remove log files
        os.remove("test_nogui_mrm_line.log")
        os.remove("test_nogui_mrm_line_dc.log")

        # remove other csv files
        os.remove("test_nogui_mrm_line-energest.csv")
        os.remove("test_nogui_mrm_line-recv.csv")
        os.remove("test_nogui_mrm_line-sent.csv")

    # combine all pdr csv
    all_filenames = [i for i in glob.glob(folder+'*-pdr.csv')]
    combined_csv = pd.concat([pd.read_csv(f) for f in all_filenames ])
    combined_csv.to_csv( folder+"/pdr.csv", index=False, encoding='utf-8-sig')

    for fname in all_filenames:
        os.remove(fname)

    # combine all dc csv
    all_filenames = [i for i in glob.glob(folder+'*-dc.csv')]
    combined_csv = pd.concat([pd.read_csv(f) for f in all_filenames ])
    combined_csv.to_csv( folder+"/dc.csv", index=False, encoding='utf-8-sig')

    for fname in all_filenames:
        os.remove(fname)

if __name__ == '__main__':
    run_udgm()
    # run_mrm()
    plot_pdr()
    plot_dc()





