from backend.models.responses import TopologyResponse, NodeDetails, SessionDetails

def build_topology_data(raw_c_json: dict) -> TopologyResponse:
    nodes_dict = {}
    links_list = []

    # 1. create nodes from global_ipv4_stats
    for ip_obj in raw_c_json.get("global_ipv4_stats", []):
        ip_addr = ip_obj.get("ip")
        if ip_addr:
            nodes_dict[ip_addr] = NodeDetails(
                ip=ip_addr,
                macs=[],
                total_packets=int(ip_obj.get("packets", 0))
            )

    # 2. Add MAC addresses to the corresponding IP nodes using mac_stats and their ipv4_history
    for mac_obj in raw_c_json.get("mac_stats", []):
        mac_addr = mac_obj.get("mac")

        for ip_hist in mac_obj.get("ipv4_history", []):
            ip_addr = ip_hist.get("ip")

            if ip_addr in nodes_dict and mac_addr not in nodes_dict[ip_addr].macs:
                nodes_dict[ip_addr].macs.append(mac_addr)

    # 3. Create links from flows and their sessions
    for flow in raw_c_json.get("flows", []):
        key = flow.get("key", {})
        src_ip = key.get("src_ip")
        dst_ip = key.get("dst_ip")
        protocol = key.get("protocol", 0)

        # Itirate over logical sessions in the flow to create links
        for sess in flow.get("sessions", []):
            session_idx = sess.get("session_idx", 0)

            link = SessionDetails(
                id=f"{src_ip}-{dst_ip}-{protocol}-{session_idx}",  # unique ID for the session
                src_ip=src_ip,
                dst_ip=dst_ip,
                start_time=sess.get("start_time", 0),
                end_time=sess.get("end_time", 0),
                protocol=protocol
            )
            links_list.append(link)

    return TopologyResponse(nodes=nodes_dict, links=links_list)