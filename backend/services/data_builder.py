from api.python.c_schemas import ParserResult
from backend.models.responses import TopologyResponse, NodeDetails, SessionDetails

def translate_tcp_state(start_state: int, end_state: int) -> dict:
    """Translates the TCP start and end states from the C engine into human-readable strings for the frontend."""
    # START STATE
    start_map = {
        0: "Idle (No Handshake)",
        1: "SYN Sent",
        2: "SYN-ACK Sent",
        3: "Handshake Complete"
    }

    # END STATE
    end_map = {
        -3: "Not Closed",
        -2: "FIN (Source)",
        -1: "FIN (Dest)",
        1: "Closed (Graceful)",
        2: "Closed (Reset/Ungraceful)"
    }

    return {
        "start_status": start_map.get(start_state, "Unknown"),
        "end_status": end_map.get(end_state, "Unknown")
    }

def build_topology_data(raw_result: ParserResult):
    nodes_dict = {}
    sessions = []
    macs_dict = {}

    # Build nodes from global IPv4 stats
    for ip_stat in raw_result.global_ipv4_stats or []:
        nodes_dict[ip_stat.ip] = {
            "id": ip_stat.ip,
            "label": ip_stat.ip,
            "ip": ip_stat.ip,
            "packets": ip_stat.packets,
            "bytes": ip_stat.bytes,
            "macs": []
        }

    # Add MAC from mac list to its corresponding IP node
    for mac_info in raw_result.mac_stats or []:
        current_mac = mac_info.mac

        if current_mac not in macs_dict:
            macs_dict[current_mac] = {
                "mac": current_mac,
                "total_packets": 0,
                "total_bytes": 0,
                "associated_ips": []
            }

        if mac_info.ipv4_data and mac_info.ipv4_data.history:
            for ip_entry in mac_info.ipv4_data.history:
                target_ip = ip_entry.ip
                if target_ip in nodes_dict:
                    # fill ip data
                    nodes_dict[target_ip]["macs"].append({
                        "mac": current_mac,
                        "packets_at_node": ip_entry.packets,
                        "bytes_at_node": ip_entry.bytes
                    })
                    #fill mac data
                    macs_dict[current_mac]["total_packets"] += ip_entry.packets
                    macs_dict[current_mac]["total_bytes"] += ip_entry.bytes
                    macs_dict[current_mac]["associated_ips"].append(target_ip)

    # Build sessions from flows
    for flow in raw_result.flows or []:
        key = flow.key
        for sess in flow.sessions:
            status = translate_tcp_state(sess.start_state, sess.end_state)

            total_packets = sum(d.packets_sent for d in sess.devices)
            total_bytes = sum(d.bytes_sent for d in sess.devices)

            sessions.append({
                "id": f"{key.src_ip}-{key.dst_ip}-{sess.session_idx}",
                "src_ip": key.src_ip,
                "dst_ip": key.dst_ip,
                "src_port": key.src_port,
                "dst_port": key.dst_port,
                "protocol": key.protocol,
                "start_time": float(sess.start_time),
                "end_time": float(sess.end_time),
                "packets": total_packets,
                "bytes": total_bytes,
                "start_status": status["start_status"],
                "end_status": status["end_status"]
            })
    return {"nodes": list(nodes_dict.values()), "links": sessions, "macs": list(macs_dict.values())} # Links is already a list