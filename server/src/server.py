import struct
import asyncio
import json
import os
import yara
import sys
import websockets.exceptions, websockets.asyncio.server

host = sys.argv[1]
port = sys.argv[2]

# Convert the windows wchar message to readable tring
def wstr_to_str(buffer):
    num_chars = len(buffer) // 2
    chars = struct.unpack('<{}H'.format(num_chars), buffer)
    return ''.join(chr(char) for char in chars)

rules = []
if not os.path.exists("./rules"):
    os.makedirs("./rules")
for yar_file in os.listdir("./rules"):
    rules.append(yara.compile("./rules/" + yar_file))
    
warnings_folder = "./warnings"
if not os.path.exists(warnings_folder):
    os.makedirs(warnings_folder)
    
log_folder = "./events"
if not os.path.exists(log_folder):
    os.makedirs(log_folder)

def yara_detect(file_path):
    matches = []
    for rule in rules:
        match = rule.match(file_path)
        if match:
            matches.append(match)
    return matches

async def handler(websocket):
    print("\033[95mServer> \033[0m\033[92mConnected to client at {}:{}\033[0m".format(
        websocket.remote_address[0], websocket.remote_address[1]))
    try:
        async for event in websocket:
            # print(event)
            event_json = json.loads(wstr_to_str(bytes(event, encoding='utf8')))
            print("\033[96m{}:{}> \033[0m{}".format(
                websocket.remote_address[0], websocket.remote_address[1], json.dumps(event_json, indent=4)))

            log_folder = "./events"
            if not os.path.exists(log_folder):
                os.makedirs(log_folder)
            current_pc_log_folder = log_folder + "/" + event_json["computerName"]
            if not os.path.exists(current_pc_log_folder):
                os.makedirs(current_pc_log_folder)
            current_event_log_file_path = current_pc_log_folder + "/event" + event_json["eventId"] + ".json"
            with open(current_event_log_file_path, "w") as f:
                f.write(json.dumps(event_json, indent=4))
                
            yara_matches = yara_detect(current_event_log_file_path)
            if len(yara_matches):
                for rule_matches in yara_matches:
                    event_json["warnings"] = [match.rule for match in rule_matches]
                
                print("\033[95mServer> \033[0m\033[93mFile {} at client {}:{} (computer {}) matches \033[0m\033[91m{}\033[0m".format(
                    event_json["imageFileDir"],
                    websocket.remote_address[0],
                    websocket.remote_address[1],
                    event_json["computerName"],
                    event_json["warnings"]))
                
                current_event_warning_file_path = warnings_folder + "/" + event_json["eventId"] + ".json"
                with open(current_event_warning_file_path, "w") as f:
                    f.write(json.dumps(event_json, indent=4))
            
            await websocket.send(event_json["eventId"])
    except websockets.exceptions.ConnectionClosedError:
        print("\033[95mServer> \033[0m\033[92mClient at {}:{} disconnected\033[0m".format(
            websocket.remote_address[0], websocket.remote_address[1]))

async def main():
    async with websockets.asyncio.server.serve(handler, host, port, ping_interval=None):
        print("\033[95mServer> \033[0m\033[92mServer is on at {}:{}\033[0m".format(
            host, port))
        await asyncio.get_running_loop().create_future()

asyncio.run(main())

