import requests
import ast

PORT = 8175
URL = f"127.0.0.1:{PORT}/"


def get(path, json_data=None):
    if json_data is None:
        json_data = dict()
    response = requests.get(url=URL+path, json=json_data)
    try:
        return ast.literal_eval(response.text)
    except Exception as e:
        return response.text


def post(path, json_data=None):
    if json_data is None:
        json_data = dict()
    response = requests.post(url=URL+path, json=json_data)
    return response.text.strip() == "1"


def put(path, json_data=None):
    if json_data is None:
        json_data = dict()
    response = requests.put(url=URL+path, json=json_data)
    return response.text.strip() == "1"


def main():
    post("startUdp")
    sink_id = get("sinkId")[0]
    post("startSink", {"sinkId": sink_id})


if __name__ == "__main__":
    main()
