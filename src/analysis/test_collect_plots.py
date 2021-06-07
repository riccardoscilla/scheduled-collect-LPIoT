import matplotlib.pyplot as plt
import pandas as pd
from collections import defaultdict
import numpy as np
import math

plt.rcParams.update({'font.size': 15})

import os
cwd = os.getcwd()
print(cwd)

def getValues(m):
    folder = "results/collect/test_udgm_"+m+"/"

    df = pd.read_csv(folder+"dc.csv", delimiter="\t")

    dc = []
    for index, row in df.iterrows():
        dc.append(row['dc'])
    dc_mean = np.mean(dc)
    dc_std = np.std(dc)

    folder = "results/collect/test_udgm_"+m+"/"
    df = pd.read_csv(folder+"pdr.csv", delimiter="\t")

    pdr = []
    for index, row in df.iterrows():
        pdr.append(row['pdr'])
    pdr_mean = np.mean(pdr)
    pdr_std = np.std(pdr)

    return dc_mean, pdr_mean, dc_std, pdr_std

modes = ["nullRDC", "nullRDC_rx20","contikiMAC", "contikiMAC_rx20"]

fig, (ax1,ax2) = plt.subplots(ncols=2, figsize=(14,7), constrained_layout=True)

nullRCD_dc, nullRCD_pdr, nullRCD_dc_error, nullRCD_pdr_error = getValues("nullRDC")
nullRCDrx20_dc, nullRCDrx20_pdr, nullRCDrx20_dc_error, nullRCDrx20_pdr_error = getValues("nullRDC_rx20")

contikiMAC_dc, contikiMAC_pdr, contikiMAC_dc_error, contikiMAC_pdr_error = getValues("contikiMAC")
contikiMACrx20_dc, contikiMACrx20_pdr, contikiMACrx20_dc_error, contikiMACrx20_pdr_error = getValues("contikiMAC_rx20")

pdr = [nullRCD_pdr,nullRCDrx20_pdr,contikiMAC_pdr,contikiMACrx20_pdr]
pdr_error = [nullRCD_pdr_error,nullRCDrx20_pdr_error,contikiMAC_pdr_error,contikiMACrx20_pdr_error]

dc = [nullRCD_dc,nullRCDrx20_dc,contikiMAC_dc,contikiMACrx20_dc]
dc_error = [nullRCD_dc_error,nullRCDrx20_dc_error,contikiMAC_dc_error,contikiMACrx20_dc_error]

print(dc)
print(dc_error)

ax1.set_title('PDR')
ax1.set_ylabel('Average PDR (%)')
width = 0.25
xpos = np.arange(len(modes))
ax1.bar(xpos, pdr, width, yerr=pdr_error, capsize=3)
ax1.set_xticks(xpos)
ax1.set_xticklabels(modes)
ax1.grid(axis="y")

ax2.set_title('DC')
ax2.set_ylabel('Average DC (%)')
width = 0.25
xpos = np.arange(len(modes))
ax2.bar(xpos, dc, width, yerr=dc_error, capsize=3)
ax2.set_xticks(xpos)
ax2.set_xticklabels(modes)
ax2.grid(axis="y")

plt.show()
fig.savefig('images/collect.png')