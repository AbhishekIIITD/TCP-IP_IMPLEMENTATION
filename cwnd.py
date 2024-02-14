from scapy.all import *
import matplotlib.pyplot as plt
from datetime import datetime

def cwnd(pcap_file):
    packets = rdpcap(pcap_file)
    
    frame_numbers = []
    timestamps = []
    
    window_sizes = []

    for i, pkt in enumerate(packets):
        if 'TCP' in pkt:
            frame_numbers.append(i)
            timestamps.append(pkt.time)
            window_sizes.append(pkt['TCP'].window)

    return  timestamps, window_sizes



pcap_file = 'h1_capture.pcap'
    
timestamps, window_sizes = cwnd(pcap_file)

# for i in range(len(frame_numbers)):
#     timestamp_str = datetime.utcfromtimestamp(float(timestamps[i])).strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
#     print(f"Frame {frame_numbers[i]} - Timestamp: {timestamp_str}, Window Size: {window_sizes[i]}")


plt.plot(timestamps, window_sizes, label='Congestion Window')
plt.xlabel('Timestamp')
plt.ylabel('Window Size')
plt.title('TCP Congestion Window')
plt.legend()
plt.xticks(rotation=45)
plt.tight_layout()
plt.show()