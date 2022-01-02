import json
import os
import requests
import subprocess
import time

import bc


class Node:
    DEFAULT_WEBSOCKET_PORT = 8080
    DEFAULT_HTTP_PORT = 8081

    SETUP_TIME = 0.25
    TEARDOWN_TIME = 0.25

    def __init__(self,
                 name,
                 config='config/default.toml',
                 websocket_host='127.0.0.1',
                 websocket_port=DEFAULT_WEBSOCKET_PORT,
                 http_host='127.0.0.1',
                 http_port=DEFAULT_HTTP_PORT,
                 with_proof_of_work=False,
                 with_transactions=False):

        self._name = name
        self._config = config
        self._websocket_host = websocket_host
        self._websocket_port = websocket_port
        self._http_host = http_host
        self._http_port = http_port
        self._with_proof_of_work = with_proof_of_work
        self._with_transactions = with_transactions

        self._api_url = f'{http_host}:{http_port}'

    def run(self):
        if self._with_proof_of_work and self._with_transactions:
            raise ValueError("Can't specify both 'proof_of_work' and 'transactions'")
        elif self._with_proof_of_work:
            node = os.getenv('NODE_POW')
        elif self._with_transactions:
            node = os.getenv('NODE_TRANS')
        else:
            node = os.getenv('NODE')

        args = [
            node,
            '--name', self._name,
            '--config', self._config,
            '--websocket-host', self._websocket_host,
            '--websocket-port', str(self._websocket_port),
            '--http-host', self._http_host,
            '--http-port', str(self._http_port),
            '--verbose'
        ]

        self._process = subprocess.Popen(args)

    def stop(self):
        self._process.terminate()

        self._process.wait()

    def add_block(self, data):
        self._api_call('blocks', 'post', data=data)

    def list_blocks(self):
        return bc.Blockchain.from_json(self._api_call('blocks', 'get'))

    def add_peer(self, node):
        data = {
            'host': node._websocket_host,
            'port': node._websocket_port
        }

        self._api_call('peers', 'post', data=data)

    def list_peers(self):
        return self._api_call('peers', 'get')

    def list_unspent_transactions(self):
        return self._api_call('transactions/unspent', 'get')

    def _api_call(self, func, method, data=None):
        method = getattr(requests, method)

        response = method(url=f'http://{self._api_url}/{func}', json=data)

        assert response.status_code == 200

        return response.json()


def make_node(node_id, *args, **kwargs):
    return Node(name=f'node{node_id}',
                websocket_port=Node.DEFAULT_WEBSOCKET_PORT + node_id * 2,
                http_port=Node.DEFAULT_HTTP_PORT + node_id * 2,
                *args,
                **kwargs)

class RunNodesContext:
    def __init__(self, nodes):
        self._nodes = nodes

    def __enter__(self):
        for node in self._nodes:
            node.run()

        time.sleep(Node.SETUP_TIME)

        if len(self._nodes) == 1:
            return self._nodes[0]

        return self._nodes

    def __exit__(self, type, value, traceback):
        for node in self._nodes:
            node.stop()

        time.sleep(Node.TEARDOWN_TIME)


def run_nodes(num_nodes, *args, **kwargs):
    nodes = [
        make_node(node_id=i, *args, **kwargs)
        for i in range(1, num_nodes + 1)
    ]

    return RunNodesContext(nodes)
