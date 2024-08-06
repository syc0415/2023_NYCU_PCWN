import re
import matplotlib.pyplot as plt

rssi_list = []

with open('Lab2 - rssi.txt', 'r') as f:
    for line in f:
        match = re.search(r'Scanned:\w+, RSSI:(-?\d+), Timestamp:\d+', line)
        if match:
            rssi = int(match.group(1))
            rssi_list.append(rssi)

plt.plot(rssi_list)
plt.xlabel('Data point')
plt.ylabel('RSSI value')
plt.show()