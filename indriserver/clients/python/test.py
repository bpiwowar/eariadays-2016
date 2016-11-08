from websocket import create_connection
import json

server = "localhost"
port = 9004
ws = create_connection("ws://%s:%d/" % (server, port))

# # Evaluate run
# import os.path
# dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
# with open(os.path.join(dir, "run.txt")) as f:
#     run = f.read()
#     ws.send(json.dumps({ "command": "submit", "name": "bpiwowar", "password": "....",
#                          "run": run }))
#     print(json.loads(ws.recv()))


# Query
queries = [{ "text": "#combine(commanders) ", "number": "1" }]
command = {"command": "query", "query": queries, "count": "100"}
ws.send(json.dumps(command))
result = json.loads(ws.recv())
print(result)


# Document content
# ws.send(json.dumps({ "command": "document", "type": "raw", "docid": 2392}))
# result = ws.recv()
# print(result)



ws.close()



