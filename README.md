<p align="center">
  <img src="logo.svg">
</p>

# BuenzliCoin - a Proof of Work Blockchain/Cryptocurrency

BuenzliCoin is a simple proof of work based blockchain/cryptocurrency written
completely from scratch in C++. It is split into two programs, `bnode` which
manages the blockchain and `bwallet` which is a command line tool that manages
wallets and interacts with `bnode` via a REST interface in order to create
transactions and mine blocks. Multiple `bnode` instances can also be linked
together and will automatically exchange transactions and blocks via WebSocket
channels, leading to distributed consensus.

## Installation

BuenzliCoin uses Conan to manage dependencies, in order to build the project run:

```
mkdir build && cd build
conan install .. --build missing
conan build ..
```

from the root of this repository. This might take a while so grab a coffee too.

## Starting `bnode`

To start a node just run `bnode` from your build directory.

You can optionally set addresses and ports of the REST and WebSocket interfaces
exposed by this node via the options:

* `--http-host`
* `--http-port`
* `--websocket-host`
* `--websocket-port`

You can also improve logging by giving the node a name with the `--name` option
and/or specifying `--verbose`.

By default the node starts out with an empty blockchain. The `--blockchain`
option can be used to specify a file containing a serialized initial
blockchain.  These files can be created for example by sending a `POST` HTTP
request to the node's `/blocks/persist` endpoint.

Additionally, some configuration parameters can be adjusted via a TOML config
file passed in via `--config`, e.g. the block generation interval, see `/config`
for inspiration. Of course all nodes in your network need to share the same
configuration in order for distributed consensus to work.

You can add "peer" nodes to the node you just started by sending a `POST` HTTP
request to the node's `/peers` endpoint.

## Full `bnode` REST interface

A running `bnode` instance can be fully controlled via a REST interface. The
following endpoints exist:

| Endpoint                    | Method | Purpose                            |
| --------------------------- | ------ | ---------------------------------- |
| `/blocks`                   | GET    | Query full blockchain              |
| `/blocks/latest`            | GET    | Query latest block                 |
| `/blocks`                   | POST   | Mine a new block                   |
| `/blocks/persist`           | POST   | Persist blockchain                 |
| `/peers`                    | GET    | Query peers                        |
| `/peers`                    | POST   | Add new peer                       |
| `/transactions/latest`      | GET    | Query transactions in latest block |
| `/transactions/unconfirmed` | GET    | Query unconfirmed transaction pool |
| `/transactions/unspent`     | GET    | Query unspent transaction outputs  |
| `/transactions`             | POST   | Add a new transaction              |

The post endpoints expect input parameters in the form of JSON dictionaries:

* `POST /blocks`:

Specify the address to which the mining reward is to be sent via `"address"`:

```
{ "address": "MFk...6wg" }
```

* `POST /blocks/persist`:

Specify the file to which the blockchain should be written via `"file"`:

```
{ "file": "blockchain.json" }
```

* `POST /peers`:

Specify ip address and port under which the peer is reachable:

```
{
  "host": "127.0.0.1"
  "port": 8333
}
```

* `POST /transactions`

Specify a transaction in the following format:

```
{
  "type": "standard                // "standard" or "reward"
  "index": 1                       // index in blockchain
  "hash": "456def..."              // hash as a hex string
  "inputs": [
    {
      "output_hash": "123abc..."   // hash of corresponding output as a hex string
      "output_index": 0            // index of corresponding output
      "signature": "deadbeef..."   // signature of "hash" as a hex string
    },
    // ...
  ]
  "outputs": [
    {
      "amount": 10                 // number of coins
      "address": "MFk...6wg"       // address of receiving wallet
    },
    // ...
  ]
}
```

In a typical workflow, `POST /peers` would first be used to connect a number of
nodes to each other, followed by several `POST /transactions` calls that create
unconfirmed transactions and `POST /blocks` calls that confirm these
transactions by adding them to the blockchain.

## Using `bwallet`

It is more convenient to use the `bwallet` program instead of the REST
interface when querying balances or creating transactions.  This program can
additionally manage wallets on your local system. `bwallet` has a number of
subcommands:

* `bwallet list`: List avaiable wallets.
* `bwallet create -name NAME`: Create a new wallet.
* `bwallet mine -to WALLET`: Mine a new block and send the reward to some wallet.
* `bwallet balance -of WALLET`: Get the balance of some wallet.
* `bwallet transfer -from WALLET -to ADDRESS`: Transfer coins from a wallet to an arbitrary address.

In order to run `bwallet`, the `BUENZLI_NODE` environment variable must be set
to the address/port under which a node is reachable, e.g.
`BUENZLI_NODE=127.0.0.1:8333`.

An example session might look like this (wallet addresses abbreviated for clarity):

```
>$ export BUENZLI_NODE=127.0.0.1:8333
>$ bwallet create -name wal1                             # create first wallet
>$ bwallet create -name wal2                             # create second wallet
>$ bwallet list                                          # list wallet names and addresses
wal1: MFk...CPA
wal2: MFk...6wg
>$ bwallet mine -to wal1                                 # mine a block and send the reward to wal1
>$ bwallet balance -of wal1                              # determine balance of wal1
50
>$ bwallet transfer -from wal1 -to MFk...6wg -amount 25  # transfer 25 coins from wal1 to wal2
>$ bwallet mine -to wal1                                 # mine another block
>$ bwallet balance -of wal1
75
>$ bwallet balance -of wal2
25
```
