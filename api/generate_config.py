import json
import os

def build_configs():
    with open('shared_config.json', 'r') as f:
        config = json.load(f)

    with open('ipc_config.h', 'w') as f_c:
        f_c.write("// AUTO-GENERATED IPC CONFIG FILE.\n\n")
        f_c.write("#ifndef IPC_CONFIG_H\n#define IPC_CONFIG_H\n\n")
        for key, value in config.items():
            if isinstance(value, str):
                f_c.write(f'#define {key} "{value}"\n')
            else:
                f_c.write(f'#define {key} {value}\n')
        f_c.write("\n#endif\n")

    with open('ipc_config.py', 'w') as f_py:
        f_py.write("# AUTO-GENERATED IPC CONFIG FILE.\n\n")
        for key, value in config.items():
            if isinstance(value, str):
                f_py.write(f'{key} = "{value}"\n')
            else:
                f_py.write(f'{key} = {value}\n')

if __name__ == "__main__":
    build_configs()
    print("[+] Configuration files generated successfully!")