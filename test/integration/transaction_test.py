from unittest import TestCase, main
import toml

from bc import ECSecp256k1PrivateKey, SHA256Hasher
from util.node import run_nodes


EC_PRIVATE_KEY1 = "MHQCAQEEILYZYhW4AeutWpQ9y5+jEY3YWR1Fohg0fdeEOow4CVVVoAcGBSuBBAAKoUQDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w"

EC_PUBLIC_KEY1 = "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAElaLbhDGtD9tOKNblgyJoYis+3kxCwFWfn+maKabqqwA+d+8RxPv5oKV0/7Y5Hj5IkPeLAl+0VAKejpNX3+F92w"

EC_PRIVATE_KEY2 = "MHQCAQEEIMhAttMFB2H70eWRmUrRqxzmr7Q0s6Oi5EzxlBKR/dCfoAcGBSuBBAAKoUQDQgAEzZAc8y92btejhFwuZfUvYNUjWIQUtPyEnHeeLjdtNCZXkN5d/7W2MHVsNZN5fW8CIQdrSWjPJGe//RXvFLakUg"

EC_PUBLIC_KEY2 = "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEzZAc8y92btejhFwuZfUvYNUjWIQUtPyEnHeeLjdtNCZXkN5d/7W2MHVsNZN5fW8CIQdrSWjPJGe//RXvFLakUg"


class TransactionTest(TestCase):
    CONFIG = 'config/transactions_test.toml'

    @classmethod
    def setUpClass(cls):
        config = toml.load(cls.CONFIG)

        cls._reward_amount = config['transaction']['reward_amount']

    def test_perform_transactions(self):
        self.maxDiff = None

        with run_nodes(num_nodes=2, config=self.CONFIG, with_transactions=True) as (node1, node2):
            node1.add_peer(node2)

            # Mine initial block
            node1.add_block(data=EC_PUBLIC_KEY1)

            for node in node1, node2:
                utxos = node.list_unspent_transactions()
                self.assertEqual(len(utxos), 1)
                self.assertDictEqual(
                    utxos[0]['output'],
                    {
                        'amount': self._reward_amount,
                        'address': EC_PUBLIC_KEY1
                    })

            # Add first transaction
            tx1 = self._create_transaction(
                {
                    'type': 'standard',
                    'index': 1,
                    'hash': None,
                    'inputs': [
                        {
                            'output_hash': utxos[0]['output_hash'],
                            'output_index': utxos[0]['output_index'],
                            'signature': None
                        }
                    ],
                    'outputs': [
                        {
                            'amount': self._reward_amount // 2,
                            'address': EC_PUBLIC_KEY1
                        },
                        {
                            'amount': self._reward_amount // 2,
                            'address': EC_PUBLIC_KEY2
                        }
                    ]
                },
                key=EC_PRIVATE_KEY1)

            node1.add_transaction(tx1)

            for node in node1, node2:
                unconfirmed = node.list_unconfirmed_transactions()
                self.assertEqual(len(unconfirmed), 1)
                self.assertDictEqual(unconfirmed[0], tx1)

            node1.add_block(EC_PUBLIC_KEY2)

            for node in node1, node2:
                utxos = node.list_unspent_transactions()
                self.assertEqual(len(utxos), 3)
                self.assertDictEqual(
                    utxos[0]['output'],
                    {
                        'amount': self._reward_amount,
                        'address': EC_PUBLIC_KEY2
                    })
                self.assertDictEqual(
                    utxos[1]['output'],
                    {
                        'amount': self._reward_amount // 2,
                        'address': EC_PUBLIC_KEY1
                    })
                self.assertDictEqual(
                    utxos[2]['output'],
                    {
                        'amount': self._reward_amount // 2,
                        'address': EC_PUBLIC_KEY2
                    })

            for node in node1, node2:
                unconfirmed = node.list_unconfirmed_transactions()
                self.assertEqual(len(unconfirmed), 0)

    @classmethod
    def _create_transaction(cls, t, key):
        cls._hash_transaction(t)
        cls._sign_transaction(t, key=key)

        return t

    @classmethod
    def _hash_transaction(cls, t):
        content = [t['index']]

        for txi in t['inputs']:
            content += [
                txi['output_hash'],
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

    @classmethod
    def _sign_transaction(cls, t, key):
        signature = cls._sign(t['hash'], key=key)

        for txi in t['inputs']:
            txi['signature'] = signature

    @classmethod
    def _hash(cls, msg):
        hasher = SHA256Hasher()

        return hasher.hash(msg)

    @classmethod
    def _sign(cls, msg, key):
        key = ECSecp256k1PrivateKey(key)

        return key.sign(msg)


if __name__ == '__main__':
    main()
