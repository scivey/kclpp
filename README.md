# KCLPP (alpha)

KCLPP is a C++ port of Amazon's Kinesis Client Library (KCL), originally written in Java.

I needed a C++ version because ~~of carefully considered technical requirements~~ deal with it.

I'm calling the current version an alpha release because it doesn't quite do what the KCL does yet.  See the feature list below.

## requirements

### libraries
KCLPP depends on the following libraries:
* AWS's [C++ SDK](https://github.com/aws/aws-sdk-cpp)
* [libevent](http://libevent.org/)
* libglog (google's logging library)

On ubuntu, you can get everything except the AWS SDK from apt:
```bash
sudo apt-get install libevent-dev \
    libgoogle-glog-dev \
    libcurl4-openssl-dev \
    libssl-dev
```

For the AWS SDK, if you're including KCLPP in a larger project you will want to follow AWS's own integration instructions in their [repo](https://github.com/aws/aws-sdk-cpp).

If you're just trying KCLPP out, you can also build the SDK from a subrepo included in this one:
```bash
    git submodule init && git submodule update
    make deps
```


### OS
KCLPP only builds on Linux.  It uses [eventfd](http://man7.org/linux/man-pages/man2/eventfd.2.html) to inform the main event loop of task completions from the thread pool.  EventFD is not available in Darwin or win32/64.  A compatibility layer probably wouldn't be too difficult, but is not at all a priority. I also reserve the right to use other Linux-only APIs in future versions.  In general, expect only Linux to work out of the box.

### Compiler
KCLPP requires a c++14 compiler because ~~generalized lambda capture makes my life a lot easier~~ I'm a disgusting hipster.


## features

### completed

#### event-based dynamodb & kinesis clients
KCLPP is based around libevent.  While AWS's default SDK build unfortunately uses blocking HTTP calls, these are all hidden behind a very low-contention thread pool.

Aside from pieces of the thread pool implementation, all of the important parts happen in a single event loop in a single thread.  This allows most classes to avoid locks around their member variables.  Where Amazon's library puts entire worker threads to sleep to implement timeouts, KCLPP's timeouts use lightweight and non-blocking timer events.

#### dynamodb-based leases
KCLPP already has a fully working implementation of the KCL's dynamodb-based lease scheme.  Most of the relevant headers are [here](/include/kclpp/leases).

Amazon's KCL uses these leases for two main purposes:
* To manage shard ownership among distributed consumers.
* To track the most recently consumed message in each shard.

In Kinesis, DynamoDB plays roughly the same role that Zookeeper has played in past versions of Kafka.  (I've heard that newer versions of Kafka track offsets directly in the brokers instead, but the idea is the same).

Like the Java KCL, KCLPP dynamically balances shard ownership across consumers.  Ownership is rebalanced when consumers go on or offline, and when Kinesis shards are divided.

#### stateful kinesis iterators
The [ShardIterator class](include/kclpp/kinesis/ShardIterator.h) is a thin layer over the Kinesis client which maintains and advances a current kinesis iterator token over a single shard.
This can be used for consuming records from a given position through the end of the shard, even without the lease or distributed coordination features. 

#### Checkpointed consumption from a single Kinesis shard
At the top level, Amazon's KCL handles stream consumption with two primary classes: `ShardConsumer` and `Worker`.  `ShardConsumer` consumes records from a single stream, feeds them to a user-implemented `RecordProcessor` class, and provides a `RecordProcessorCheckpointer` object which the `RecordProcessor` can call to save its progress.  Aside from a little bit of polished, `ShardConsumer` is already implemented [here](/src/kclpp/clientlib/worker/ShardConsumer.cpp).

#### Lots of tests
See the [/src/test directory](src/test), e.g. [here](src/test/functional/test_KCLKinesisClient_functional.cpp).


### remaining
* The `Worker` class from Amazon's KCL, which monitors lease activity and starts/stops `ShardConsumer` and `RecordProcessors` in response.
* Demultiplexing of user-level records from Amazon's `Kinesis Producer Library`.
