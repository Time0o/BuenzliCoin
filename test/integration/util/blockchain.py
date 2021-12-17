def assertBlockchainValid(test, blockchain, length):
    test.assertEqual(blockchain.empty(), length == 0)
    test.assertEqual(blockchain.length(), length)
    test.assertEqual(blockchain.valid(), length != 0)


def assertBlockchainValues(test, blockchain, values_expected):
    assertBlockchainValid(test, blockchain, len(values_expected))

    values_actual = [b.data() for b in blockchain.all_blocks()]

    test.assertEqual(values_actual, values_expected)


def assertBlockDifficulties(test, blocks, difficulties_expected):
    def hex_to_bin(i_hex):
        i_int = int(i_hex, base=16)
        i_bin = bin(i_int)[2:].zfill(len(i_hex) * 4)

        return i_bin

    def difficulty(h_hex):
        h_bin = hex_to_bin(h_hex)

        return (len(h_bin) - len(h_bin.lstrip('0'))) * 4

    difficulties_actual = [difficulty(b.hash()) for b in blocks]

    test.assertTrue(
        all(d1 >= d2 for d1, d2 in zip(difficulties_actual, difficulties_expected)))
