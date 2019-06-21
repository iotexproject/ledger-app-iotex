Transaction Specification
-------------------------

### Format

Transactions passed to the Ledger device will be the IoTeX's action-core message serialized by protobuf.

Action-core may enclose many kinds of actions. But only two actions are supported by Ledger device:
 transfer and execution of smart contract. The protobuf definition of action core is [here](https://github.com/iotexproject/iotex-proto/blob/master/proto/types/action.proto).


#### Examples


#### Display Logic

