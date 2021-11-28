import unittest

from util.node import Node, run_node


class SingleNodeTest(unittest.TestCase):
    def test_build_blockchain(self):
        with run_node() as node:
            # Initial (empty) blockchain
            blocks = node.list_blocks()

            self.assertTrue(blocks.empty())
            self.assertEqual(blocks.length(), 0)
            self.assertFalse(blocks.valid())

            # Add first block
            node.add_block('first')
            blocks = node.list_blocks()

            self.assertFalse(blocks.empty())
            self.assertEqual(blocks.length(), 1)
            self.assertTrue(blocks.valid())

            self.assertEqual(['first'], [block.data() for block in blocks])

            # Add second block
            node.add_block('second')
            blocks = node.list_blocks()

            self.assertFalse(blocks.empty())
            self.assertEqual(blocks.length(), 2)
            self.assertTrue(blocks.valid())

            self.assertEqual(['first', 'second'], [block.data() for block in blocks])


if __name__ == '__main__':
    unittest.main()
