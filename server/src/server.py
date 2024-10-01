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
for yar_file in os.listdir("./rules"):
    rules.append(yara.compile("./rules/" + yar_file))

def yara_detect(file_path):
    for rule in rules:
        matches = rule.match(file_path)
    return matches

async def handler(websocket):
    print("Connected to client at {}:{}".format(websocket.remote_address[0], websocket.remote_address[1]))
    try:
        async for event in websocket:
            # print(event)
            event_json = json.loads(wstr_to_str(bytes(event, encoding='utf8')))
            print("{}:{}> {}".format(websocket.remote_address[0], websocket.remote_address[1], event_json))

            log_folder = "./events"
            if not os.path.exists(log_folder):
                os.makedirs(log_folder)
            current_pc_log_folder = log_folder + "/" + event_json["computerName"]
            if not os.path.exists(current_pc_log_folder):
                os.makedirs(current_pc_log_folder)
            current_event_log_file_path = current_pc_log_folder + "/event" + event_json["eventId"] + ".json"
            with open(current_event_log_file_path, "w") as f:
                f.write(json.dumps(event_json))
                
            yara_matches = yara_detect(current_event_log_file_path)
            if yara_matches:
                print("Server> File {} at client {}:{} (computer {}) may contains {}".format(
                    event_json["imageFileDir"],
                    websocket.remote_address[0],
                    websocket.remote_address[1],
                    event_json["computerName"],
                    yara_matches))
            
            await websocket.send(event_json["eventId"])
    except websockets.exceptions.ConnectionClosedError:
        print("Client at {}:{} disconnected".format(websocket.remote_address[0], websocket.remote_address[1]))

async def main():
    async with websockets.asyncio.server.serve(handler, host, port, ping_interval=None):
        print("Server is on at {}:{}".format(host, port))
        await asyncio.get_running_loop().create_future()

asyncio.run(main())

