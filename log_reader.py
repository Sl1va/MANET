import re
from collections import defaultdict

# Regular expressions to match log lines
# packet_sent_re = re.compile(r'OLSR node (\S+) sending a OLSR packet')
# packet_received_re = re.compile(r'OLSR node (\S+) received a OLSR packet')
hello_received_re = re.compile(r'(\d+\.\d+)s OLSR node (\S+) received HELLO message of size \d+')
tc_received_re = re.compile(r'(\d+\.\d+)s OLSR node (\S+) received TC message of size \d+')
mid_received_re = re.compile(r'(\d+\.\d+)s OLSR node (\S+) received MID message of size \d+')
hna_received_re = re.compile(r'(\d+\.\d+)s OLSR node (\S+) received HNA message of size \d+')
hello_sent = re.compile(r'\[node (\d+)\] (\d+)s : Sending HELLO packet')
tc_sent = re.compile(r'\[node (\d+)\] (\d+)s : Sending TC packet')
mid_sent = re.compile(r'\[node (\d+)\] (\d+)s : Sending MID packet')
hna_sent = re.compile(r'\[node (\d+)\] (\d+)s : Sending HNA packet')
time_re = re.compile(r'(\d+\.\d+)s')

# Initialize counters
total_packets_sent = 0
total_packets_received = 0
packets_sent_by_node = defaultdict(int)
packets_received_by_node = defaultdict(int)
packets_sent_per_second = defaultdict(int)
packets_received_per_second = defaultdict(int)
packets_sent_per_second_by_node = defaultdict(lambda: defaultdict(int))
packets_received_per_second_by_node = defaultdict(lambda: defaultdict(int))

# Read the log file
with open('olsr.group.log', 'r') as file:
    for line in file:
        hello_received_match = hello_received_re.search(line)
        tc_received_match = tc_received_re.search(line)
        mid_received_match = mid_received_re.search(line)
        hna_received_match = hna_received_re.search(line)

        hello_sent_match = hello_sent.search(line)
        tc_sent_match = tc_sent.search(line)
        mid_sent_match = mid_sent.search(line)
        hna_sent_match = hna_sent.search(line)

        time_match = time_re.search(line)
        
        if hello_received_match:
            time = hello_received_match.group(1)
            node = hello_received_match.group(2)
            second = int(float(time))
            
            total_packets_received += 1
            packets_received_by_node[node] += 1
            packets_received_per_second[second] += 1
            packets_received_per_second_by_node[second][node] += 1
        
        if tc_received_match:
            time = tc_received_match.group(1)
            node = tc_received_match.group(2)
            second = int(float(time))
            
            total_packets_received += 1
            packets_received_by_node[node] += 1
            packets_received_per_second[second] += 1
            packets_received_per_second_by_node[second][node] += 1
        
        if mid_received_match:
            time = mid_received_match.group(1)
            node = mid_received_match.group(2)
            second = int(float(time))
            
            total_packets_received += 1
            packets_received_by_node[node] += 1
            packets_received_per_second[second] += 1
            packets_received_per_second_by_node[second][node] += 1
        
        if hna_received_match:
            time = hna_received_match.group(1)
            node = hna_received_match.group(2)
            second = int(float(time))
            
            total_packets_received += 1
            packets_received_by_node[node] += 1
            packets_received_per_second[second] += 1
            packets_received_per_second_by_node[second][node] += 1
        
        if hello_sent_match:
            time = hello_sent_match.group(2)
            node = hello_sent_match.group(1)
            second = int(float(time))
            
            total_packets_sent += 1
            packets_sent_by_node[node] += 1
            packets_sent_per_second[second] += 1
            packets_sent_per_second_by_node[second][node] += 1
        
        if tc_sent_match:
            time = tc_sent_match.group(2)
            node = tc_sent_match.group(1)
            second = int(float(time))
            
            total_packets_sent += 1
            packets_sent_by_node[node] += 1
            packets_sent_per_second[second] += 1
            packets_sent_per_second_by_node[second][node] += 1

        if mid_sent_match:
            time = mid_sent_match.group(2)
            node = mid_sent_match.group(1)
            second = int(float(time))
            
            total_packets_sent += 1
            packets_sent_by_node[node] += 1
            packets_sent_per_second[second] += 1
            packets_sent_per_second_by_node[second][node] += 1
        
        if hna_sent_match:
            time = hna_sent_match.group(2)
            node = hna_sent_match.group(1)
            second = int(float(time))
            
            total_packets_sent += 1
            packets_sent_by_node[node] += 1
            packets_sent_per_second[second] += 1
            packets_sent_per_second_by_node[second][node] += 1
        


# Generate the report
report = {
    'total_packets_sent': total_packets_sent,
    'total_packets_received': total_packets_received,
    'packets_sent_by_node': dict(packets_sent_by_node),
    'packets_received_by_node': dict(packets_received_by_node),
    'packets_sent_per_second': dict(packets_sent_per_second),
    'packets_received_per_second': dict(packets_received_per_second),
    'packets_sent_per_second_by_node': {k: dict(v) for k, v in packets_sent_per_second_by_node.items()},
    'packets_received_per_second_by_node': {k: dict(v) for k, v in packets_received_per_second_by_node.items()},
}

import json
print(json.dumps(report, indent=4))
