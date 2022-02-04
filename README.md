<p align="center">
  <img src="logo.svg">
</p>

# BuenzliCoin - a Proof of Work Blockchain/Cryptocurrency

BuenzliCoin is a simple proof of work based blockchain/cryptocurrency written
completely from scratch in C++. It is split into two programs, `bnode` which
manages the blockchain and `bwallet` which is a command line tool that manages
wallets and interacts with `bnode` via a REST interface in order to create
transactions and mine blocks. Multiple `bnode` instance can also be linked
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

You can optionally set addresses and ports of the REST and websocket interfaces
exposed by this node via the options:

* `--http-host`
* `--http-port`
* `--websocket-host`
* `--websocket-port`

You can also improve logging by giving the node a name with the `--name` option
and/or specifying `--verbose`.

Additionally, some configuration parameters can be adjusted via a TOML config
file passed in via `--config`, e.g. the block generation interval, see `/config`
for inspiration. Of course all nodes in your network need to share the same
configuration in order for distributed consensus to work.

You can add "peer" nodes to the node you just started by sending a `POST` HTTP
request to the node's `/peers` endpoint whose body is:

```json
{
  "host": "$some_ip_address"
  "port": "$some_port"
}
```

## Using `bwallet`

Although all necessary functionality for managing transactions is also available
via a rest interface, it is more convenient to use to `bwallet` program to do so.
This program can additionally manage wallets on your local system. `bwallet` has
a number of subcommands:

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
