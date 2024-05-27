import re
from collections import defaultdict

# Regular expressions to match log lines
# packet_sent_re = re.compile(r'OLSR node (\S+) sending a OLSR packet')
# packet_received_re = re.compile(r'OLSR node (\S+) received a OLSR packet')
omg_received_re = re.compile(r'\[node (\d+)\] (\d+) s: Receive OMG Packet')
omg_sent = re.compile(r'\[node (\d+)\] (\d+) s: Send OMG Packet')

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
with open('batmand.ind.log', 'r') as file:
    for line in file:
        omg_received_match = omg_received_re.search(line)

        omg_sent_match = omg_sent.search(line)

        time_match = time_re.search(line)

        if omg_received_match:
            time = omg_received_match.group(2)
            node = omg_received_match.group(1)
            second = int(float(time))
            
            total_packets_received += 1
            packets_received_by_node[node] += 1
            packets_received_per_second[second] += 1
            packets_received_per_second_by_node[second][node] += 1
        
        if omg_sent_match:
            time = omg_sent_match.group(2)
            node = omg_sent_match.group(1)
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
