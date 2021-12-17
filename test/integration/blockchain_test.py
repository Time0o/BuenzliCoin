import unittest

from util.blockchain import assertBlockchainValid, assertBlockchainValues
from util.node import run_nodes


class BlockchainTest(unittest.TestCase):
    def test_build_blockchain(self):
        with run_nodes(num_nodes=1) as node:
            # Initial (empty) blockchain
            assertBlockchainValues(self, node.list_blocks(), [])

            # Add first block
            node.add_block('first')
            assertBlockchainValues(self, node.list_blocks(), ['first'])

            # Add second block
            node.add_block('second')
            assertBlockchainValues(self, node.list_blocks(), ['first', 'second'])

            # Add third block
            node.add_block('third')
            assertBlockchainValues(self, node.list_blocks(), ['first', 'second', 'third'])

    def test_broadcast_blocks(self):
        with run_nodes(num_nodes=3) as (node1, node2, node3):
            node1.add_peer(node2)
            node1.add_peer(node3)

            node1.add_block('node1')

            assertBlockchainValues(self, node2.list_blocks(), ['node1'])
            assertBlockchainValues(self, node3.list_blocks(), ['node1'])

    def test_query_blocks(self):
        with run_nodes(num_nodes=3) as (node1, node2, node3):
            node1.add_block('node1')

            node2.add_peer(node1)

            assertBlockchainValues(self, node2.list_blocks(), ['node1'])

            node3.add_peer(node2)

            assertBlockchainValues(self, node3.list_blocks(), ['node1'])

    def test_query_blockchains(self):
        with run_nodes(num_nodes=2) as (node1, node2):
            node1.add_block('first')
            node1.add_block('second')
            node1.add_block('third')

            node2.add_peer(node1)

            assertBlockchainValues(self, node2.list_blocks(), ['first', 'second', 'third'])

        with run_nodes(num_nodes=3) as (node1, node2, node3):
            node1.add_block('first')
            node1.add_block('second')

            node1.add_peer(node2)
            node1.add_peer(node3)

            node1.add_block('third')

            assertBlockchainValues(self, node2.list_blocks(), ['first', 'second', 'third'])
            assertBlockchainValues(self, node3.list_blocks(), ['first', 'second', 'third'])


if __name__ == '__main__':
    unittest.main()
