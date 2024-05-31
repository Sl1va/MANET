import os
import json
from collections import defaultdict

#seeds = [1, 5, 7, 13, 56, 43, 66, 12, 87, 132]
seeds = [1]

home_dir = os.path.expanduser("~")

file_prefixes = ["/logs/batman.group.", "/logs/batman.ind.", "/logs/olsr.group.", "/logs/olsr.ind."]
file_prefixes = [home_dir + i for i in file_prefixes]


def sum_by_seconds(var1, var2):
    for t in range(0, 101):
        if str(t) not in var2: continue
        var1[str(t)] += var2[str(t)]


def sum_by_nodes(var1, var2):
    for n in range(0, 25):
        if str(n) not in var2: continue
        var1[str(n)] += var2[str(n)]


def sum_by_seconds_by_nodes(var1, var2):
    for t in range(0, 101):
        if str(t) not in var2: continue
        for n in range(0, 25):
            if str(n) not in var2[str(t)]: continue
            var1[str(t)][str(n)] += var2[str(t)][str(n)]


for file_prefix in file_prefixes:
    total_packets_sent = 0
    total_packets_received = 0
    packets_sent_by_node = defaultdict(int)
    packets_received_by_node = defaultdict(int)
    packets_sent_per_second = defaultdict(int)
    packets_received_per_second = defaultdict(int)
    packets_sent_per_second_by_node = defaultdict(lambda: defaultdict(int))
    packets_received_per_second_by_node = defaultdict(lambda: defaultdict(int))

    save_filename = file_prefix + "norm.json"

    for seed in seeds:
        filename = file_prefix + str(seed) + ".json"

        with open(filename, "r") as f:
            data = json.load(f, 'utf-8')

            total_packets_sent += data["total_packets_sent"]
            total_packets_received += data["total_packets_received"]

            sum_by_nodes(packets_sent_by_node, data["packets_sent_by_node"])
            sum_by_nodes(packets_received_by_node, data["packets_received_by_node"])

            sum_by_seconds(packets_sent_per_second, data["packets_sent_per_second"])
            sum_by_seconds(packets_received_per_second, data["packets_received_per_second"])

            sum_by_seconds_by_nodes(packets_sent_per_second_by_node, data["packets_sent_per_second_by_node"])
            sum_by_seconds_by_nodes(packets_received_per_second_by_node, data["packets_received_per_second_by_node"])

    total_packets_sent /= 10
    total_packets_received /= 10

    for n in range(0, 25):
        packets_sent_by_node[str(n)] /= 10
        packets_received_by_node[str(n)] /= 10
    
    for t in range(0, 101):
        packets_sent_per_second[str(t)] /= 10
        packets_received_per_second[str(t)] /= 10
    
    for t in range(0, 101):
        for n in range(0, 25):
            packets_sent_per_second_by_node[str(t)][str(n)] /= 10
            packets_received_per_second_by_node[str(t)][str(n)] /= 10


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

    f = open(save_filename, "w")
    f.write(json.dumps(report, indent=4))
    f.close()
