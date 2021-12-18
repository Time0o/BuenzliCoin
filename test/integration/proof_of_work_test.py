from datetime import timedelta
from math import floor, log2
from unittest import TestCase, main
import toml

from util.blockchain import assertBlockDifficulties
from util.node import run_nodes


class ProofOfWorkTest(TestCase):
    CONFIG = 'config/proof_of_work_test.toml'

    @classmethod
    def setUpClass(cls):
        config = toml.load(cls.CONFIG)

        cls._time_expected = timedelta(seconds=config['block_gen']['time_expected'])
        cls._difficulty_init = config['block_gen']['difficulty_init']
        cls._difficulty_adjust_after = config['block_gen']['difficulty_adjust_after']
        cls._difficulty_adjust_factor_limit = config['block_gen']['difficulty_adjust_factor_limit']

    def test_build_blockchain(self):
        NUM_ADJUSTMENTS = 5

        with run_nodes(num_nodes=1, config=self.CONFIG, proof_of_work=True) as node:
            for _ in range(NUM_ADJUSTMENTS):
                self._append_blocks(node)

    def _append_blocks(self, node):
        for _ in range(self._difficulty_adjust_after):
            node.add_block('data')

        blockchain = node.list_blocks()

        if blockchain.length() == self._difficulty_adjust_after:
            self._difficulty_expected = self._difficulty_init
            self._difficulty_expected_log2 = floor(log2(self._difficulty_expected))
            return

        blocks = blockchain.all_blocks()
        blocks_adjust = blocks[-(2 * self._difficulty_adjust_after + 1):-self._difficulty_adjust_after]
        blocks_new = blocks[-self._difficulty_adjust_after:]

        total_time_expected = len(blocks_adjust) * self._time_expected
        total_time_actual = blocks_adjust[-1].timestamp() - blocks_adjust[0].timestamp()

        difficulty_adjust_factor = min(self._difficulty_adjust_factor_limit,
                                       total_time_expected / total_time_actual)

        self._difficulty_expected *= difficulty_adjust_factor
        self._difficulty_expected_log2 = floor(log2(self._difficulty_expected))

        assertBlockDifficulties(self,
                                blocks_new,
                                [self._difficulty_expected_log2] * len(blocks_new))


if __name__ == '__main__':
    main()
