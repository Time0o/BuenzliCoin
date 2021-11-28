import json
import os
import requests
import subprocess
import time

import bc


class Node:
    DEFAULT_WEBSOCKET_PORT = 8080
    DEFAULT_HTTP_PORT = 8081

    SETUP_TIME = 0.5

    def __init__(self,
                 websocket_host='127.0.0.1',
                 websocket_port=DEFAULT_WEBSOCKET_PORT,
                 http_host='127.0.0.1',
                 http_port=DEFAULT_HTTP_PORT):

        self._websocket_host = websocket_host
        self._websocket_port = websocket_port
        self._http_host = http_host
        self._http_port = http_port

        self._api_url = f'{http_host}:{http_port}'

    def run(self):
        args = [
            os.getenv('NODE'),
            '--websocket-host', self._websocket_host,
            '--websocket-port', str(self._websocket_port),
            '--http-host', self._http_host,
            '--http-port', str(self._http_port)
        ]

        self._process = subprocess.Popen(args)

    def stop(self):
        self._process.terminate()

    def add_block(self, data):
        self._api_call('add-block', 'post', data=data)

    def list_blocks(self):
        return bc.Blockchain.from_json(self._api_call('list-blocks', 'get'))

    def _api_call(self, func, method, data=None):
        method = getattr(requests, method)

        response = method(url=f'http://{self._api_url}/{func}', json=data)

        print('FOO', response.status_code, flush=True) # TODO
        assert response.status_code == 200

        return response.json()


class NodeContext:
    def __init__(self, *args, **kwargs):
        self._node = Node(*args, **kwargs)

    def __enter__(self):
        self._node.run()

        time.sleep(Node.SETUP_TIME)

        return self._node

    def __exit__(self, type, value, traceback):
        self._node.stop()


def run_node(node_id=0):
    return NodeContext(websocket_port=Node.DEFAULT_WEBSOCKET_PORT + node_id,
                       http_port=Node.DEFAULT_HTTP_PORT + node_id)


def run_nodes(num_nodes):
    return (run_node(node_id) for node_id in range(num_nodes))
