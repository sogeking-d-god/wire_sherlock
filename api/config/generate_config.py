import json
import os

def build_configs():
    with open('shared_config.json', 'r') as f:
        config = json.load(f)

    c_lines = []
    py_lines = []

    for key, value in config.get("IPC", {}).items():
        c_lines.append((key, value))
        py_lines.append((key, value))

    for key, value in config.get("COMMANDS", {}).items():
        c_lines.append((f"CMD_{key}", value))
        py_lines.append((f"CMD_{key}", value))

    for key, value in config.get("STATUS", {}).items():
        c_lines.append((f"STATUS_{key}", value))
        py_lines.append((f"STATUS_{key}", value))

    with open('ipc_config.h', 'w') as f_c:
        f_c.write("// AUTO-GENERATED FILE. DO NOT EDIT.\n")
        f_c.write("#ifndef IPC_CONFIG_H\n#define IPC_CONFIG_H\n\n")
        for key, value in c_lines:
            if isinstance(value, str):
                f_c.write(f'#define {key} "{value}"\n')
            else:
                f_c.write(f'#define {key} {value}\n')
        f_c.write("\n#endif // IPC_CONFIG_H\n")

    with open('ipc_config.py', 'w') as f_py:
        f_py.write("# AUTO-GENERATED FILE. DO NOT EDIT.\n\n")
        for key, value in py_lines:
            if isinstance(value, str):
                f_py.write(f'{key} = "{value}"\n')
            else:
                f_py.write(f'{key} = {value}\n')

if __name__ == "__main__":
    build_configs()
    print("[+] Configuration files generated successfully!")