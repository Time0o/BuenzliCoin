import unittest

from util.node import Node, run_node, run_nodes


class BlockchainTest(unittest.TestCase):
    def test_build_blockchain(self):
        with run_node() as node:
            # Initial (empty) blockchain
            self.assertBlockchainEqual([], node)

            # Add first block
            node.add_block('first')
            self.assertBlockchainEqual(['first'], node)

            # Add second block
            node.add_block('second')
            self.assertBlockchainEqual(['first', 'second'], node)

            # Add third block
            node.add_block('third')
            self.assertBlockchainEqual(['first', 'second', 'third'], node)

    def test_broadcast_blocks(self):
        with run_nodes(num_nodes=3) as (node1, node2, node3):
            node1.add_peer(node2)
            node1.add_peer(node3)

            node1.add_block('node1')

            self.assertBlockchainEqual(['node1'], node2)
            self.assertBlockchainEqual(['node1'], node3)

    def test_query_blocks(self):
        with run_nodes(num_nodes=3) as (node1, node2, node3):
            node1.add_block('node1')

            node2.add_peer(node1)

            self.assertBlockchainEqual(['node1'], node2)

            node3.add_peer(node2)

            self.assertBlockchainEqual(['node1'], node3)

    def test_query_blockchains(self):
        with run_nodes(num_nodes=2) as (node1, node2):
            node1.add_block('first')
            node1.add_block('second')
            node1.add_block('third')

            node2.add_peer(node1)

            self.assertBlockchainEqual(['first', 'second', 'third'], node2)

        with run_nodes(num_nodes=3) as (node1, node2, node3):
            node1.add_block('first')
            node1.add_block('second')

            node1.add_peer(node2)
            node1.add_peer(node3)

            node1.add_block('third')

            self.assertBlockchainEqual(['first', 'second', 'third'], node2)
            self.assertBlockchainEqual(['first', 'second', 'third'], node3)

    def assertBlockchainEqual(self, values, node):
        blockchain = node.list_blocks()

        self.assertEqual(blockchain.empty(), not bool(values))
        self.assertEqual(blockchain.length(), len(values))
        self.assertEqual(blockchain.valid(), bool(values))

        self.assertEqual(values, [b.data() for b in blockchain.all_blocks()])


if __name__ == '__main__':
    unittest.main()
