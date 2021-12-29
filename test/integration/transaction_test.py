from unittest import TestCase, main
import toml

from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS

from util.node import run_nodes


EC_PRIVATE_KEY1 = "LYZYhW4AeutWpQ9y5+jEY3YWR1Fohg0fdeEOow4CVVVoAcGBSuBBAAKoUQDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w=="

EC_PUBLIC_KEY1 = "laLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w=="


class TransactionTest(TestCase):
    CONFIG = 'config/transactions_test.toml'

    @classmethod
    def setUpClass(cls):
        config = toml.load(cls.CONFIG)

        cls._reward_amount = config['transaction']['reward_amount']

    def test_perform_transactions(self):
        with run_nodes(num_nodes=1, config=self.CONFIG, with_transactions=True) as node:
            tx0 = self._create_transaction(
                {
                    'type': 'reward',
                    'index': 0,
                    'hash': None,
                    'inputs': [],
                    'outputs': [
                        {
                            'amount': self._reward_amount,
                            'address': EC_PUBLIC_KEY1
                        }
                    ]
                },
                key=EC_PRIVATE_KEY1)

            node.add_block(tx0)

            utxos0 = node.transactions.unspent_outputs()

            self.assertEqual(len(utxos0), 1)

            self.assertDictEqual(
                utxos0[0],
                {
                    'transaction_index': tx0['index'],
                    'transaction_hash': tx0['hash'],
                    'output_index': 0,
                    'output': {
                        'amount': self._reward_amount,
                        'address': EC_PUBLIC_KEY1
                    }
                })

    @classmethod
    def _create_transaction(cls, t, key):
        cls._hash_transaction(t)
        cls._sign_transaction(t, key=key)

        return t

    @classmethod
    def _hash_transaction(cls, t):
        content = []

        for txi in t['inputs']:
            content += [
                txi['transaction_index'],
                txi['transaction_hash'],
                txi['output_index']
            ]

        for txo in t['outputs']:
            content += [
                txo['amount'],
                txo['address']
            ]

        content = ''.join(str(c) for c in content)

        h = cls._hash(content)

        t['hash'] = h

        for txi in t['inputs']:
            txi['transaction_hash'] = h

    @classmethod
    def _sign_transaction(cls, t, key):
        pass # XXX

    @classmethod
    def _hash(cls, msg):
        return SHA256.new(msg.encode()).hexdigest()


if __name__ == '__main__':
    main()
