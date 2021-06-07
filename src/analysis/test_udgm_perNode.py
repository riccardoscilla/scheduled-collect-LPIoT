import matplotlib.pyplot as plt
import pandas as pd
from collections import defaultdict
import numpy as np
import math

plt.rcParams.update({'font.size': 15})

import os
cwd = os.getcwd()
print(cwd)

def getValues(p):
    folder = "results/distLoss/test_udgm_distLoss_50_changeP/"

    df = pd.read_csv(folder+"pdr"+p+".csv", delimiter="\t")
    pdr_dict = defaultdict(list)

    pdr = []
    pdr_error = []

    for index, row in df.iterrows():
        pdr_dict[row['node']].append(row['pdr'])

    for p in pdr_dict:
        pdr.append(np.mean(pdr_dict[p]))
        pdr_error.append(np.std(pdr_dict[p]))    

    return pdr,pdr_error

ids = ["2","3","4","5","6","7","8","9"]
fig, (ax1) = plt.subplots(ncols=1, figsize=(7,7), constrained_layout=True)

pdr3,pdr3_error= getValues("3")
pdr7,pdr7_error= getValues("7")

ax1.set_title('PDR per node id')
ax1.set_ylabel('Average PDR (%)')
ax1.set_xlabel('node_id')
width = 0.4
xpos = np.arange(len(ids))
ax1.bar(xpos-width/2, pdr3, width, yerr=pdr3_error, capsize=3, label="N=50 P=3")
ax1.bar(xpos+width/2, pdr7, width, yerr=pdr7_error, capsize=3, label="N=50 P=7")
ax1.set_xticks(xpos)
ax1.set_xticklabels(ids)
ax1.grid(axis="y")
ax1.legend(loc=1)


plt.show()
fig.savefig('images/perNode.png')