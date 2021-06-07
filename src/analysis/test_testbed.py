import matplotlib.pyplot as plt
import pandas as pd
from collections import defaultdict
import numpy as np
import math

plt.rcParams.update({'font.size': 15})

import os
cwd = os.getcwd()
print(cwd)

def getValues(m,contikiMAC=False):
    if contikiMAC:
        folder = "results/testbed/contikiMAC/"
    else:
        folder = "results/testbed/testbed_50_p"+m+"/"

    df = pd.read_csv(folder+"test-dc.csv", delimiter="\t")

    dc = []
    for index, row in df.iterrows():
        dc.append(row['dc'])
        
    dc_mean = np.mean(dc)
    dc_std = np.std(dc)

    df = pd.read_csv(folder+"test-pdr.csv", delimiter="\t")

    pdr = []
    for index, row in df.iterrows():
        pdr.append(row['pdr'])
    pdr_mean = np.mean(pdr)
    pdr_std = np.std(pdr)

    return dc_mean, pdr_mean, dc_std, pdr_std

p = ["5","10","15","contikiMAC"]

fig, (ax1,ax2) = plt.subplots(ncols=2, figsize=(14,7), constrained_layout=True)

dc5, pdr5, dc5_error, pdr5_error = getValues("5")
dc10, pdr10, dc10_error, pdr10_error = getValues("10")
dc15, pdr15, dc15_error, pdr15_error = getValues("15")
dcContikiMAC, pdrContikiMAC, dcContikiMAC_error, pdrContikiMAC_error = getValues(None,True)

pdr = [pdr5,pdr10,pdr15,pdrContikiMAC]
pdr_error = [pdr5_error,pdr10_error,pdr15_error,pdrContikiMAC_error]

dc = [dc5,dc10,dc15,dcContikiMAC]
dc_error = [dc5_error,dc10_error,dc15_error,dcContikiMAC_error]

ax1.set_title('PDR')
ax1.set_ylabel('Average PDR (%)')
width = 0.25
xpos = np.arange(len(p))
ax1.bar(xpos, pdr, width, yerr=pdr_error, capsize=3)
ax1.set_xticks(xpos)
ax1.set_xticklabels(p)
ax1.grid(axis="y")

ax2.set_title('DC')
ax2.set_ylabel('Average DC (%)')
width = 0.25
xpos = np.arange(len(p))
ax2.bar(xpos, dc, width, yerr=dc_error, capsize=3)
ax2.set_xticks(xpos)
ax2.set_xticklabels(p)
ax2.grid(axis="y")

plt.show()
fig.savefig('images/testbed.png')