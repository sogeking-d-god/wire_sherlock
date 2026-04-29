
class TopologyBuilder:
    @staticmethod
    def build(c_engine_data):
        nodes = {}
        edges = []

        for mac_info in c_engine_data.get("mac_stats", []):
            mac_addr = mac_info.get("mac")
            for ip_info in mac_info.get("ipv4_history", []):
                ip_addr = ip_info.get("ip")
                if ip_addr not in nodes:
                    nodes[ip_addr] = {
                        "id": ip_addr,
                        "mac": mac_addr,
                        "label": ip_addr,
                        "packets": ip_info.get("packets", 0),
                        "bytes": ip_info.get("bytes", 0)
                    }

        for flow in c_engine_data.get("flows", []):
            key = flow.get("key", {})
            src_ip = key.get("src_ip")
            dst_ip = key.get("dst_ip")
            src_port = key.get("src_port")
            dst_port = key.get("dst_port")
            proto = key.get("protocol")

            for session in flow.get("sessions", []):
                s_idx = session.get("session_idx")

                edge_id = f"{src_ip}:{src_port}->{dst_ip}:{dst_port}_{proto}_{s_idx}"

                edges.append({
                    "id": edge_id,
                    "source": src_ip,
                    "target": dst_ip,
                    "src_port": src_port,
                    "dst_port": dst_port,
                    "protocol": proto,
                    "start_time": session.get("start_time"),
                    "end_time": session.get("end_time"),
                    "bytes": sum(dev.get("bytes_sent", 0) for dev in session.get("devices", []))
                })

        return {
            "nodes": list(nodes.values()),
            "edges": edges
        }