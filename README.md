# stackhpc-io-motifs
Application motif benchmarks for IO characterisation

## Compilation
These tools have been developed using GCC on Linux.  To use the Ceph
object support, the `librados` development libraries should be installed.

To compile the motifs, simply use `make`.

## Motif 1: Small scattered file IO
This motif is intended as a tool for exploring different methods for writing
out large numbers of small files, and then reading them back.  In this
scenario, a large number of concurrent threads are performing sequential
file accesses, making it difficult for any individual thread to cover the
read access latency of the storage cluster.

In these situations, a performant storage platform will cope with high
request concurrency but respond to each with low latency.  The distribution
of request latency is also of some interest, but average latency and aggregate
IOPS are often more important than the long tail.

### Invocation
Concurrency is achieved through large numbers of sequential processes.
The motif executable can be invoked with a parameter for the number of child
processes to create within the process group.  For coordination across
multiple test client nodes, use another orchestration mechanism such,
Kubernetes jobs or (more simply) remote shell invocation.

Example invocation (for low-level Ceph RADOS API):

```
./motif_1 -t /tmp/motif-traces -v INFO -c 100000 -n 10000 -p 24 -S RADOS -- --pool benchmark -n client.admin --keyring=ceph.client.admin.keyring
```

In this example, the parameters are:

| Parameter | Notes |
|:----------|:------|
| `-t /tmp/motif-traces` | Directory for access timing trace records.  Every object accessed is recorded.  Each process writes its own file. |
| `-v INFO`              | Set the verbosity level for logging. |
| `-c 100000`            | Number of objects to write.  Each process will write this number of objects. |
| `-n 10000`             | Number of objects to read back.  Each process will read and validate this number of objects.  Each process will only read back objects that it has written. |
| `-p 24`                | Number of child processes to create. |
| `-S RADOS`             | Storage driver to use, in this case the low-level object API of Ceph (RADOS). |
| `--`                   | Supply further driver-specific parameters. |

